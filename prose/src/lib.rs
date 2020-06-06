// lib.rs

/// Type-erased errors.
pub type BoxError = std::boxed::Box<dyn
	std::error::Error   // must implement Error to satisfy ?
	+ std::marker::Send // needed for threads
	+ std::marker::Sync // needed for threads
>;


pub type MarkdownText = Vec<MarkdownInline>;

#[derive(Clone,Debug,PartialEq)]
pub enum Markdown {
    Heading(usize, MarkdownText),
	OrderedList(Vec<MarkdownText>),
    UnorderedList(Vec<MarkdownText>),
	Line(MarkdownText),
    Codeblock(String),
}

#[derive(Clone,Debug,PartialEq)]
pub enum MarkdownInline {
	Link(String, String),
    Image(String, String),
	InlineCode(String),
	Bold(String),
    Italic(String),
	Plaintext(String),
}


pub(self) mod parsers {
    use super::Markdown;
	use super::MarkdownText;
	use super::MarkdownInline;

	fn match_boldtext(i: &str) -> nom::IResult<&str, &str> {
		nom::sequence::delimited(
			nom::bytes::complete::tag("**"),
			nom::bytes::complete::is_not("**"),
			nom::bytes::complete::tag("**"),
		)(i)
	}

	fn match_italics(i: &str) -> nom::IResult<&str, &str> {
		nom::sequence::delimited(
			nom::bytes::complete::tag("*"),
			nom::bytes::complete::is_not("*"),
			nom::bytes::complete::tag("*"),
		)(i)
	}

	fn match_inline_code(i: &str) -> nom::IResult<&str, &str> {
		nom::sequence::delimited(
			nom::bytes::complete::tag("`"),
			nom::bytes::complete::is_not("`"),
			nom::bytes::complete::tag("`"),
		)(i)
	}

	fn match_link(i: &str) -> nom::IResult<&str, (&str, &str)> {
		nom::sequence::pair(
			nom::sequence::delimited(
				nom::bytes::complete::tag("["),
				nom::bytes::complete::is_not("]"),
				nom::bytes::complete::tag("]"),
			),
			nom::sequence::delimited(
				nom::bytes::complete::tag("("),
				nom::bytes::complete::is_not(")"),
				nom::bytes::complete::tag(")"),
			)
		)(i)
	}

	fn match_image(i: &str) -> nom::IResult<&str, (&str, &str)> {
		nom::sequence::pair(
			nom::sequence::delimited(
				nom::bytes::complete::tag("!["),
				nom::bytes::complete::is_not("]"),
				nom::bytes::complete::tag("]"),
			),
			nom::sequence::delimited(
				nom::bytes::complete::tag("("),
				nom::bytes::complete::is_not(")"),
				nom::bytes::complete::tag(")"),
			)
		)(i)
	}

	fn match_plaintext(i: &str) -> nom::IResult<&str, String> {
		nom::combinator::map(
			nom::multi::many1(
				nom::sequence::preceded(
					nom::combinator::not(
						nom::branch::alt((
							nom::bytes::complete::tag("*"),
							nom::bytes::complete::tag("`"),
							nom::bytes::complete::tag("["),
							nom::bytes::complete::tag("!["),
							nom::bytes::complete::tag("\n")
						))
					),
					nom::bytes::complete::take(1u8),
				)
			),
			|vec| vec.join("")
		)(i)
	}

	fn match_markdown_inline(i: &str) -> nom::IResult<&str, MarkdownInline> {
		nom::branch::alt((
			nom::combinator::map(
				match_italics,
				|s: &str| MarkdownInline::Italic(s.to_string())
			),
			nom::combinator::map(
				match_inline_code,
				|s: &str| MarkdownInline::InlineCode(s.to_string())
			),
			nom::combinator::map(
				match_boldtext,
				|s: &str| MarkdownInline::Bold(s.to_string())
			),
			nom::combinator::map(
				match_image,
				|(tag, link): (&str, &str)| MarkdownInline::Image(tag.to_string(), link.to_string())
			),
			nom::combinator::map(
				match_link,
				|(tag, link): (&str, &str)| MarkdownInline::Link(tag.to_string(), link.to_string())
			),
			nom::combinator::map(
				match_plaintext,
				|s| MarkdownInline::Plaintext(s)
			),
		))(i)
	}

	fn match_markdown_text(i: &str) -> nom::IResult<&str, MarkdownText> {
		nom::sequence::terminated(
			nom::multi::many0(
				match_markdown_inline
			),
			nom::bytes::complete::tag("\n")
		)(i)
	}

	// this guy matches the literal character #
	fn match_header_tag(i: &str) -> nom::IResult<&str, usize>  {
		nom::combinator::map(
			nom::sequence::terminated(
				nom::bytes::complete::take_while1(|c| c == '#'),
				nom::bytes::complete::tag(" ")
			),
			|s: &str| s.len()
		)(i)
	}

	// this combines a tuple of the header tag and the rest of the line
	fn match_header(i: &str) -> nom::IResult<&str, (usize, MarkdownText)> {
		nom::sequence::tuple(( match_header_tag, match_markdown_text ))(i)
	}

	fn match_unordered_list_tag(i: &str) -> nom::IResult<&str, &str> {
		nom::sequence::terminated(
			nom::bytes::complete::tag("-"),
			nom::bytes::complete::tag(" ")
		)(i)
	}

	fn match_unordered_list_element(i: &str) -> nom::IResult<&str, MarkdownText> {
		nom::sequence::preceded(
			match_unordered_list_tag,
			match_markdown_text
		)(i)
	}

	fn match_unordered_list(i: &str)  -> nom::IResult<&str, Vec<MarkdownText>> {
		nom::multi::many1(match_unordered_list_element)(i)
	}

	fn match_ordered_list_tag(i: &str) -> nom::IResult<&str, &str> {
		nom::sequence::terminated(
			nom::sequence::terminated(
				nom::bytes::complete::take_while1(|d| nom::character::is_digit(d as u8)),
				nom::bytes::complete::tag(".")
			),
			nom::bytes::complete::tag(" ")
		)(i)
	}

	fn match_ordered_list_element(i: &str) -> nom::IResult<&str, MarkdownText> {
		nom::sequence::preceded(
			match_ordered_list_tag,
			match_markdown_text
		)(i)
	}

	fn match_ordered_list(i: &str)  -> nom::IResult<&str, Vec<MarkdownText>> {
		nom::multi::many1(match_ordered_list_element)(i)
	}

	fn match_markdown(i: &str) -> nom::IResult<&str, Markdown> {
		nom::branch::alt((
			nom::combinator::map(
				match_header,
				|e| Markdown::Heading(e.0, e.1)
			),
			nom::combinator::map(
				match_unordered_list,
				|e| Markdown::UnorderedList(e)
			),
			nom::combinator::map(
				match_ordered_list,
				|e| Markdown::OrderedList(e)
			),
		))(i)
	}

    #[cfg(test)]
    mod tests {
        use super::*;

		#[test]
		fn test_match_italics() {
			assert_eq!(match_italics("*here is italic*"), Ok(("", "here is italic")));
			assert_eq!(match_italics("*here is italic"), Err(nom::Err::Error(("", nom::error::ErrorKind::Tag))));
			assert_eq!(match_italics("here is italic*"), Err(nom::Err::Error(("here is italic*", nom::error::ErrorKind::Tag))));
			assert_eq!(match_italics("here is italic"), Err(nom::Err::Error(("here is italic", nom::error::ErrorKind::Tag))));
			assert_eq!(match_italics("*"), Err(nom::Err::Error(("", nom::error::ErrorKind::IsNot))));
			assert_eq!(match_italics("**"), Err(nom::Err::Error(("*", nom::error::ErrorKind::IsNot))));
			assert_eq!(match_italics(""), Err(nom::Err::Error(("", nom::error::ErrorKind::Tag))));
			assert_eq!(match_italics("**we are doing bold**"), Err(nom::Err::Error(("*we are doing bold**", nom::error::ErrorKind::IsNot))));
		}

		#[test]
		fn test_match_boldtext() {
			assert_eq!(match_boldtext("**here is bold**"), Ok(("", "here is bold")));
			assert_eq!(match_boldtext("**here is bold"), Err(nom::Err::Error(("", nom::error::ErrorKind::Tag))));
			assert_eq!(match_boldtext("here is bold**"), Err(nom::Err::Error(("here is bold**", nom::error::ErrorKind::Tag))));
			assert_eq!(match_boldtext("here is bold"), Err(nom::Err::Error(("here is bold", nom::error::ErrorKind::Tag))));
			assert_eq!(match_boldtext("****"), Err(nom::Err::Error(("**", nom::error::ErrorKind::IsNot))));
			assert_eq!(match_boldtext("**"), Err(nom::Err::Error(("", nom::error::ErrorKind::IsNot))));
			assert_eq!(match_boldtext("*"), Err(nom::Err::Error(("*", nom::error::ErrorKind::Tag))));
			assert_eq!(match_boldtext(""), Err(nom::Err::Error(("", nom::error::ErrorKind::Tag))));
			assert_eq!(match_boldtext("*this is italic*"), Err(nom::Err::Error(("*this is italic*", nom::error::ErrorKind::Tag))));
		}

		#[test]
		fn test_match_inline_code() {
			assert_eq!(match_boldtext("**here is bold**\n"), Ok(("\n", "here is bold")));
			assert_eq!(match_inline_code("`here is code"), Err(nom::Err::Error(("", nom::error::ErrorKind::Tag))));
			assert_eq!(match_inline_code("here is code`"), Err(nom::Err::Error(("here is code`", nom::error::ErrorKind::Tag))));
			assert_eq!(match_inline_code("``"), Err(nom::Err::Error(("`", nom::error::ErrorKind::IsNot))));
			assert_eq!(match_inline_code("`"), Err(nom::Err::Error(("", nom::error::ErrorKind::IsNot))));
			assert_eq!(match_inline_code(""), Err(nom::Err::Error(("", nom::error::ErrorKind::Tag))));
		}

		#[test]
		fn test_match_link() {
			assert_eq!(match_link("[title](https://www.example.com)"), Ok(("", ("title", "https://www.example.com"))));
			assert_eq!(match_inline_code(""), Err(nom::Err::Error(("", nom::error::ErrorKind::Tag))));
		}

		#[test]
		fn test_match_image() {
			assert_eq!(match_image("![alt text](image.jpg)"), Ok(("", ("alt text", "image.jpg"))));
			assert_eq!(match_inline_code(""), Err(nom::Err::Error(("", nom::error::ErrorKind::Tag))));
		}

		#[test]
		fn test_match_plaintext() {
			assert_eq!(match_plaintext("1234567890"), Ok(("", String::from("1234567890"))));
			assert_eq!(match_plaintext("oh my gosh!"), Ok(("", String::from("oh my gosh!"))));
			assert_eq!(match_plaintext("oh my gosh!["), Ok(("![", String::from("oh my gosh"))));
			assert_eq!(match_plaintext("oh my gosh!*"), Ok(("*", String::from("oh my gosh!"))));
			assert_eq!(match_plaintext("*bold babey bold*"), Err(nom::Err::Error(("*bold babey bold*", nom::error::ErrorKind::Not))));
			assert_eq!(match_plaintext("[link babey](and then somewhat)"), Err(nom::Err::Error(("[link babey](and then somewhat)", nom::error::ErrorKind::Not))));
			assert_eq!(match_plaintext("`codeblock for bums`"), Err(nom::Err::Error(("`codeblock for bums`", nom::error::ErrorKind::Not))));
			assert_eq!(match_plaintext("![ but wait theres more](jk)"), Err(nom::Err::Error(("![ but wait theres more](jk)", nom::error::ErrorKind::Not))));
			assert_eq!(match_plaintext("here is plaintext"), Ok(("", String::from("here is plaintext"))));
			assert_eq!(match_plaintext("here is plaintext!"), Ok(("", String::from("here is plaintext!"))));
			assert_eq!(match_plaintext("here is plaintext![image starting"), Ok(("![image starting", String::from("here is plaintext"))));
			assert_eq!(match_plaintext("here is plaintext\n"), Ok(("\n", String::from("here is plaintext"))));
			assert_eq!(match_plaintext("*here is italic*"), Err(nom::Err::Error(("*here is italic*", nom::error::ErrorKind::Not))));
			assert_eq!(match_plaintext("**here is bold**"), Err(nom::Err::Error(("**here is bold**", nom::error::ErrorKind::Not))));
			assert_eq!(match_plaintext("`here is code`"), Err(nom::Err::Error(("`here is code`", nom::error::ErrorKind::Not))));
			assert_eq!(match_plaintext("[title](https://www.example.com)"), Err(nom::Err::Error(("[title](https://www.example.com)", nom::error::ErrorKind::Not))));
			assert_eq!(match_plaintext("![alt text](image.jpg)"), Err(nom::Err::Error(("![alt text](image.jpg)", nom::error::ErrorKind::Not))));
			assert_eq!(match_plaintext(""), Err(nom::Err::Error(("", nom::error::ErrorKind::Eof))));
		}

		#[test]
		fn test_match_markdown_inline() {
			assert_eq!(match_markdown_inline("*here is italic*"), Ok(("", MarkdownInline::Italic(String::from("here is italic")))));
			assert_eq!(match_markdown_inline("**here is bold**"), Ok(("", MarkdownInline::Bold(String::from("here is bold")))));
			assert_eq!(match_markdown_inline("`here is code`"), Ok(("", MarkdownInline::InlineCode(String::from("here is code")))));
			assert_eq!(match_markdown_inline("[title](https://www.example.com)"), Ok(("", (MarkdownInline::Link(String::from("title"), String::from("https://www.example.com"))))));
			assert_eq!(match_markdown_inline("![alt text](image.jpg)"), Ok(("", (MarkdownInline::Image(String::from("alt text"), String::from("image.jpg"))))));
			assert_eq!(match_markdown_inline("here is plaintext!"), Ok(("", MarkdownInline::Plaintext(String::from("here is plaintext!")))));
			assert_eq!(match_markdown_inline("here is some plaintext *but what if we italicize?"), Ok(("*but what if we italicize?", MarkdownInline::Plaintext(String::from("here is some plaintext ")))));
			assert_eq!(match_markdown_inline("here is some plaintext \n*but what if we italicize?"), Ok(("\n*but what if we italicize?", MarkdownInline::Plaintext(String::from("here is some plaintext ")))));
			assert_eq!(match_markdown_inline("\n"), Err(nom::Err::Error(("\n", nom::error::ErrorKind::Not))));
			assert_eq!(match_markdown_inline(""), Err(nom::Err::Error(("", nom::error::ErrorKind::Eof))));
		}

		#[test]
		fn test_match_markdown_text() {
			assert_eq!( match_markdown_text("\n"), Ok(("", vec![])) );
			assert_eq!(
				match_markdown_text("here is some plaintext\n"),
				Ok(("", vec![MarkdownInline::Plaintext(String::from("here is some plaintext"))]))
			);
			assert_eq!(
				match_markdown_text("here is some plaintext *but what if we italicize?*\n"),
				Ok(("", vec![
					MarkdownInline::Plaintext(String::from("here is some plaintext ")),
					MarkdownInline::Italic(String::from("but what if we italicize?")),
				]))
			);
			assert_eq!(
				match_markdown_text("here is some plaintext *but what if we italicize?* I guess it doesnt **matter** in my `code`\n"),
				Ok(("", vec![
					MarkdownInline::Plaintext(String::from("here is some plaintext ")),
					MarkdownInline::Italic(String::from("but what if we italicize?")),
					MarkdownInline::Plaintext(String::from(" I guess it doesnt ")),
					MarkdownInline::Bold(String::from("matter")),
					MarkdownInline::Plaintext(String::from(" in my ")),
					MarkdownInline::InlineCode(String::from("code")),
				]))
			);
			assert_eq!(
				match_markdown_text("here is some plaintext *but what if we italicize?*\n"),
				Ok(("", vec![
					MarkdownInline::Plaintext(String::from("here is some plaintext ")),
					MarkdownInline::Italic(String::from("but what if we italicize?")),
				]))
			);
			assert_eq!(
				match_markdown_text("here is some plaintext *but what if we italicize?"),
				Err(nom::Err::Error(("*but what if we italicize?", nom::error::ErrorKind::Tag)))
				// Ok(("*but what if we italicize?", vec![MarkdownInline::Plaintext(String::from("here is some plaintext "))]))
			);
		}

		#[test]
        fn test_match_header_tag() {
			assert_eq!(match_header_tag("# "), Ok(("", 1)));
            assert_eq!(match_header_tag("### "), Ok(("", 3)));
            assert_eq!(match_header_tag("# h1"), Ok(("h1", 1)));
			assert_eq!(match_header_tag("# h1"), Ok(("h1", 1)));
			assert_eq!(match_header_tag(" "), Err(nom::Err::Error((" ", nom::error::ErrorKind::TakeWhile1))));
			assert_eq!(match_header_tag("#"), Err(nom::Err::Error(("", nom::error::ErrorKind::Tag))));
		}

		#[test]
		fn test_match_header() {
			assert_eq!(match_header("# h1\n"), Ok(("", (1, vec![MarkdownInline::Plaintext(String::from("h1"))]))));
			assert_eq!(match_header("## h2\n"), Ok(("", (2, vec![MarkdownInline::Plaintext(String::from("h2"))]))));
			assert_eq!(match_header("###  h3\n"), Ok(("", (3, vec![MarkdownInline::Plaintext(String::from(" h3"))]))));
			assert_eq!(match_header("###h3"), Err(nom::Err::Error(("h3", nom::error::ErrorKind::Tag))));
			assert_eq!(match_header("###"), Err(nom::Err::Error(("", nom::error::ErrorKind::Tag))));
			assert_eq!(match_header(""), Err(nom::Err::Error(("", nom::error::ErrorKind::TakeWhile1))));
			assert_eq!(match_header("#"), Err(nom::Err::Error(("", nom::error::ErrorKind::Tag))));
			assert_eq!(match_header("# \n"), Ok(("", (1, vec![]))));
		}

		#[test]
		fn test_match_unordered_list_tag() {
			assert_eq!(match_unordered_list_tag("- "), Ok(("", "-")));
			assert_eq!(match_unordered_list_tag("- and some more"), Ok(("and some more", "-")));
			assert_eq!(match_unordered_list_tag("-"), Err(nom::Err::Error(("", nom::error::ErrorKind::Tag))));
			assert_eq!(match_unordered_list_tag("-and some more"), Err(nom::Err::Error(("and some more", nom::error::ErrorKind::Tag))));
			assert_eq!(match_unordered_list_tag("--"), Err(nom::Err::Error(("-", nom::error::ErrorKind::Tag))));
			assert_eq!(match_unordered_list_tag(""), Err(nom::Err::Error(("", nom::error::ErrorKind::Tag))));
		}

		#[test]
		fn test_match_unordered_list_element() {
			assert_eq!(match_unordered_list_element("- this is an element\n"), Ok(("", vec![MarkdownInline::Plaintext(String::from("this is an element"))])));
			assert_eq!(match_unordered_list_element("- this is an element\n- this is another element\n"), Ok(("- this is another element\n", vec![MarkdownInline::Plaintext(String::from("this is an element"))])));
			assert_eq!(match_unordered_list_element(""), Err(nom::Err::Error(("", nom::error::ErrorKind::Tag))));
			assert_eq!(match_unordered_list_element("- \n"), Ok(("", vec![])));
			assert_eq!(match_unordered_list_element("- "), Err(nom::Err::Error(("", nom::error::ErrorKind::Tag))));
			assert_eq!(match_unordered_list_element("-"), Err(nom::Err::Error(("", nom::error::ErrorKind::Tag))));
		}

		#[test]
		fn test_match_unordered_list() {
			assert_eq!(match_unordered_list("- this is an element\n"), Ok(("", vec![vec![MarkdownInline::Plaintext(String::from("this is an element"))]])));
			assert_eq!(match_unordered_list("- this is an element\n- here is another\n"), Ok(("", vec![vec![MarkdownInline::Plaintext(String::from("this is an element"))], vec![MarkdownInline::Plaintext(String::from("here is another"))]])));
		}

		#[test]
		fn test_match_ordered_list_tag() {
			assert_eq!(match_ordered_list_tag("1. "), Ok(("", "1")));
			assert_eq!(match_ordered_list_tag("1234567. "), Ok(("", "1234567")));
			assert_eq!(match_ordered_list_tag("3. and some more"), Ok(("and some more", "3")));
			assert_eq!(match_ordered_list_tag("1"), Err(nom::Err::Error(("", nom::error::ErrorKind::Tag))));
			assert_eq!(match_ordered_list_tag("1.and some more"), Err(nom::Err::Error(("and some more", nom::error::ErrorKind::Tag))));
			assert_eq!(match_ordered_list_tag("1111."), Err(nom::Err::Error(("", nom::error::ErrorKind::Tag))));
			assert_eq!(match_ordered_list_tag(""), Err(nom::Err::Error(("", nom::error::ErrorKind::TakeWhile1))));
		}

		#[test]
		fn test_match_ordered_list_element() {
			assert_eq!(match_ordered_list_element("1. this is an element\n"), Ok(("", vec![MarkdownInline::Plaintext(String::from("this is an element"))])));
			assert_eq!(match_ordered_list_element("1. this is an element\n1. here is another\n"), Ok(("1. here is another\n", vec![MarkdownInline::Plaintext(String::from("this is an element"))])));
			assert_eq!(match_ordered_list_element(""), Err(nom::Err::Error(("", nom::error::ErrorKind::TakeWhile1))));
			assert_eq!(match_ordered_list_element(""), Err(nom::Err::Error(("", nom::error::ErrorKind::TakeWhile1))));
			assert_eq!(match_ordered_list_element("1. \n"), Ok(("", vec![])));
			assert_eq!(match_ordered_list_element("1. "), Err(nom::Err::Error(("", nom::error::ErrorKind::Tag))));
			assert_eq!(match_ordered_list_element("1."), Err(nom::Err::Error(("", nom::error::ErrorKind::Tag))));
		}

		#[test]
		fn test_match_ordered_list() {
			assert_eq!(match_ordered_list("1. this is an element\n"), Ok(("", vec![vec![MarkdownInline::Plaintext(String::from("this is an element"))]])));
			assert_eq!(match_ordered_list("1. this is an element\n2. here is another\n"), Ok(("", vec![vec!(MarkdownInline::Plaintext(String::from("this is an element"))), vec![MarkdownInline::Plaintext(String::from("here is another"))]])));
		}
    }
}
