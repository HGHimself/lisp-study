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
	use nom::{
		character::is_digit,
		bytes::complete::{tag, is_not, take_while1, take},
		branch::{alt},
		sequence::{delimited, pair, preceded, terminated, tuple},
		multi::{many0, many1},
		combinator::{map, not},
		error::{ErrorKind},
		Err::Error,
		IResult,
	};

	fn match_boldtext(i: &str) -> IResult<&str, &str> {
		delimited( tag("**"), is_not("**"), tag("**") )(i)
	}

	fn match_italics(i: &str) -> IResult<&str, &str> {
		delimited( tag("*"), is_not("*"), tag("*") )(i)
	}

	fn match_inline_code(i: &str) -> IResult<&str, &str> {
		delimited( tag("`"), is_not("`"), tag("`") )(i)
	}

	fn match_link(i: &str) -> IResult<&str, (&str, &str)> {
		pair(
			delimited( tag("["), is_not("]"), tag("]") ),
			delimited( tag("("), is_not(")"), tag(")") )
		)(i)
	}

	fn match_image(i: &str) -> IResult<&str, (&str, &str)> {
		pair(
			delimited( tag("!["), is_not("]"), tag("]") ),
			delimited( tag("("), is_not(")"), tag(")") )
		)(i)
	}

	// we want to match many things that are not any of our specail tags
	// but since we have no tools available to match and consume in the negative case (without regex)
	// we need to match against our tags, then consume one char
	// we repeat this until we run into one of our special characters
	// then we join our array of characters into a String
	fn match_plaintext(i: &str) -> IResult<&str, String> {
		map(
			many1(
				preceded(
					not( alt(( tag("*"), tag("`"), tag("["), tag("!["), tag("\n") )) ),
					take(1u8),
				)
			),
			|vec| vec.join("")
		)(i)
	}

	fn match_markdown_inline(i: &str) -> IResult<&str, MarkdownInline> {
		alt((
			map( match_italics, |s: &str| MarkdownInline::Italic(s.to_string()) ),
			map( match_inline_code, |s: &str| MarkdownInline::InlineCode(s.to_string()) ),
			map( match_boldtext, |s: &str| MarkdownInline::Bold(s.to_string()) ),
			map( match_image, |(tag, link): (&str, &str)| MarkdownInline::Image(tag.to_string(), link.to_string()) ),
			map( match_link, |(tag, link): (&str, &str)| MarkdownInline::Link(tag.to_string(), link.to_string()) ),
			map( match_plaintext, |s| MarkdownInline::Plaintext(s) ),
		))(i)
	}

	fn match_markdown_text(i: &str) -> IResult<&str, MarkdownText> {
		terminated( many0( match_markdown_inline ), tag("\n") )(i)
	}

	// this guy matches the literal character #
	fn match_header_tag(i: &str) -> IResult<&str, usize>  {
		map(
			terminated( take_while1(|c| c == '#'), tag(" ") ),
			|s: &str| s.len()
		)(i)
	}

	// this combines a tuple of the header tag and the rest of the line
	fn match_header(i: &str) -> IResult<&str, (usize, MarkdownText)> {
		tuple(( match_header_tag, match_markdown_text ))(i)
	}

	fn match_unordered_list_tag(i: &str) -> IResult<&str, &str> {
		terminated( tag("-"), tag(" ") )(i)
	}

	fn match_unordered_list_element(i: &str) -> IResult<&str, MarkdownText> {
		preceded( match_unordered_list_tag, match_markdown_text )(i)
	}

	fn match_unordered_list(i: &str)  -> IResult<&str, Vec<MarkdownText>> {
		many1(match_unordered_list_element)(i)
	}

	fn match_ordered_list_tag(i: &str) -> IResult<&str, &str> {
		terminated(
			terminated( take_while1(|d| is_digit(d as u8)), tag(".") ),
			tag(" ")
		)(i)
	}

	fn match_ordered_list_element(i: &str) -> IResult<&str, MarkdownText> {
		preceded( match_ordered_list_tag, match_markdown_text )(i)
	}

	fn match_ordered_list(i: &str) -> IResult<&str, Vec<MarkdownText>> {
		many1(match_ordered_list_element)(i)
	}

	fn match_code_block(i: &str) -> IResult<&str, &str> {

	}

	fn match_markdown(i: &str) -> IResult<&str, Vec<Markdown>> {
		many1(
			alt((
				map( match_header, |e| Markdown::Heading(e.0, e.1) ),
				map( match_unordered_list, |e| Markdown::UnorderedList(e) ),
				map( match_ordered_list, |e| Markdown::OrderedList(e) ),
				map( match_markdown_text, |e| Markdown::Line(e) )
			))
		)(i)
	}

    #[cfg(test)]
    mod tests {
        use super::*;

		#[test]
		fn test_match_italics() {
			assert_eq!(match_italics("*here is italic*"), Ok(("", "here is italic")));
			assert_eq!(match_italics("*here is italic"), Err(Error(("", ErrorKind::Tag))));
			assert_eq!(match_italics("here is italic*"), Err(Error(("here is italic*", ErrorKind::Tag))));
			assert_eq!(match_italics("here is italic"), Err(Error(("here is italic", ErrorKind::Tag))));
			assert_eq!(match_italics("*"), Err(Error(("", ErrorKind::IsNot))));
			assert_eq!(match_italics("**"), Err(Error(("*", ErrorKind::IsNot))));
			assert_eq!(match_italics(""), Err(Error(("", ErrorKind::Tag))));
			assert_eq!(match_italics("**we are doing bold**"), Err(Error(("*we are doing bold**", ErrorKind::IsNot))));
		}

		#[test]
		fn test_match_boldtext() {
			assert_eq!(match_boldtext("**here is bold**"), Ok(("", "here is bold")));
			assert_eq!(match_boldtext("**here is bold"), Err(Error(("", ErrorKind::Tag))));
			assert_eq!(match_boldtext("here is bold**"), Err(Error(("here is bold**", ErrorKind::Tag))));
			assert_eq!(match_boldtext("here is bold"), Err(Error(("here is bold", ErrorKind::Tag))));
			assert_eq!(match_boldtext("****"), Err(Error(("**", ErrorKind::IsNot))));
			assert_eq!(match_boldtext("**"), Err(Error(("", ErrorKind::IsNot))));
			assert_eq!(match_boldtext("*"), Err(Error(("*", ErrorKind::Tag))));
			assert_eq!(match_boldtext(""), Err(Error(("", ErrorKind::Tag))));
			assert_eq!(match_boldtext("*this is italic*"), Err(Error(("*this is italic*", ErrorKind::Tag))));
		}

		#[test]
		fn test_match_inline_code() {
			assert_eq!(match_boldtext("**here is bold**\n"), Ok(("\n", "here is bold")));
			assert_eq!(match_inline_code("`here is code"), Err(Error(("", ErrorKind::Tag))));
			assert_eq!(match_inline_code("here is code`"), Err(Error(("here is code`", ErrorKind::Tag))));
			assert_eq!(match_inline_code("``"), Err(Error(("`", ErrorKind::IsNot))));
			assert_eq!(match_inline_code("`"), Err(Error(("", ErrorKind::IsNot))));
			assert_eq!(match_inline_code(""), Err(Error(("", ErrorKind::Tag))));
		}

		#[test]
		fn test_match_link() {
			assert_eq!(match_link("[title](https://www.example.com)"), Ok(("", ("title", "https://www.example.com"))));
			assert_eq!(match_inline_code(""), Err(Error(("", ErrorKind::Tag))));
		}

		#[test]
		fn test_match_image() {
			assert_eq!(match_image("![alt text](image.jpg)"), Ok(("", ("alt text", "image.jpg"))));
			assert_eq!(match_inline_code(""), Err(Error(("", ErrorKind::Tag))));
		}

		#[test]
		fn test_match_plaintext() {
			assert_eq!(match_plaintext("1234567890"), Ok(("", String::from("1234567890"))));
			assert_eq!(match_plaintext("oh my gosh!"), Ok(("", String::from("oh my gosh!"))));
			assert_eq!(match_plaintext("oh my gosh!["), Ok(("![", String::from("oh my gosh"))));
			assert_eq!(match_plaintext("oh my gosh!*"), Ok(("*", String::from("oh my gosh!"))));
			assert_eq!(match_plaintext("*bold babey bold*"), Err(Error(("*bold babey bold*", ErrorKind::Not))));
			assert_eq!(match_plaintext("[link babey](and then somewhat)"), Err(Error(("[link babey](and then somewhat)", ErrorKind::Not))));
			assert_eq!(match_plaintext("`codeblock for bums`"), Err(Error(("`codeblock for bums`", ErrorKind::Not))));
			assert_eq!(match_plaintext("![ but wait theres more](jk)"), Err(Error(("![ but wait theres more](jk)", ErrorKind::Not))));
			assert_eq!(match_plaintext("here is plaintext"), Ok(("", String::from("here is plaintext"))));
			assert_eq!(match_plaintext("here is plaintext!"), Ok(("", String::from("here is plaintext!"))));
			assert_eq!(match_plaintext("here is plaintext![image starting"), Ok(("![image starting", String::from("here is plaintext"))));
			assert_eq!(match_plaintext("here is plaintext\n"), Ok(("\n", String::from("here is plaintext"))));
			assert_eq!(match_plaintext("*here is italic*"), Err(Error(("*here is italic*", ErrorKind::Not))));
			assert_eq!(match_plaintext("**here is bold**"), Err(Error(("**here is bold**", ErrorKind::Not))));
			assert_eq!(match_plaintext("`here is code`"), Err(Error(("`here is code`", ErrorKind::Not))));
			assert_eq!(match_plaintext("[title](https://www.example.com)"), Err(Error(("[title](https://www.example.com)", ErrorKind::Not))));
			assert_eq!(match_plaintext("![alt text](image.jpg)"), Err(Error(("![alt text](image.jpg)", ErrorKind::Not))));
			assert_eq!(match_plaintext(""), Err(Error(("", ErrorKind::Eof))));
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
			assert_eq!(match_markdown_inline("\n"), Err(Error(("\n", ErrorKind::Not))));
			assert_eq!(match_markdown_inline(""), Err(Error(("", ErrorKind::Eof))));
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
				Err(Error(("*but what if we italicize?", ErrorKind::Tag)))
				// Ok(("*but what if we italicize?", vec![MarkdownInline::Plaintext(String::from("here is some plaintext "))]))
			);
		}

		#[test]
        fn test_match_header_tag() {
			assert_eq!(match_header_tag("# "), Ok(("", 1)));
            assert_eq!(match_header_tag("### "), Ok(("", 3)));
            assert_eq!(match_header_tag("# h1"), Ok(("h1", 1)));
			assert_eq!(match_header_tag("# h1"), Ok(("h1", 1)));
			assert_eq!(match_header_tag(" "), Err(Error((" ", ErrorKind::TakeWhile1))));
			assert_eq!(match_header_tag("#"), Err(Error(("", ErrorKind::Tag))));
		}

		#[test]
		fn test_match_header() {
			assert_eq!(match_header("# h1\n"), Ok(("", (1, vec![MarkdownInline::Plaintext(String::from("h1"))]))));
			assert_eq!(match_header("## h2\n"), Ok(("", (2, vec![MarkdownInline::Plaintext(String::from("h2"))]))));
			assert_eq!(match_header("###  h3\n"), Ok(("", (3, vec![MarkdownInline::Plaintext(String::from(" h3"))]))));
			assert_eq!(match_header("###h3"), Err(Error(("h3", ErrorKind::Tag))));
			assert_eq!(match_header("###"), Err(Error(("", ErrorKind::Tag))));
			assert_eq!(match_header(""), Err(Error(("", ErrorKind::TakeWhile1))));
			assert_eq!(match_header("#"), Err(Error(("", ErrorKind::Tag))));
			assert_eq!(match_header("# \n"), Ok(("", (1, vec![]))));
			assert_eq!(match_header("# test"), Err(Error(("", ErrorKind::Tag))));
		}

		#[test]
		fn test_match_unordered_list_tag() {
			assert_eq!(match_unordered_list_tag("- "), Ok(("", "-")));
			assert_eq!(match_unordered_list_tag("- and some more"), Ok(("and some more", "-")));
			assert_eq!(match_unordered_list_tag("-"), Err(Error(("", ErrorKind::Tag))));
			assert_eq!(match_unordered_list_tag("-and some more"), Err(Error(("and some more", ErrorKind::Tag))));
			assert_eq!(match_unordered_list_tag("--"), Err(Error(("-", ErrorKind::Tag))));
			assert_eq!(match_unordered_list_tag(""), Err(Error(("", ErrorKind::Tag))));
		}

		#[test]
		fn test_match_unordered_list_element() {
			assert_eq!(match_unordered_list_element("- this is an element\n"), Ok(("", vec![MarkdownInline::Plaintext(String::from("this is an element"))])));
			assert_eq!(match_unordered_list_element("- this is an element\n- this is another element\n"), Ok(("- this is another element\n", vec![MarkdownInline::Plaintext(String::from("this is an element"))])));
			assert_eq!(match_unordered_list_element(""), Err(Error(("", ErrorKind::Tag))));
			assert_eq!(match_unordered_list_element("- \n"), Ok(("", vec![])));
			assert_eq!(match_unordered_list_element("- "), Err(Error(("", ErrorKind::Tag))));
			assert_eq!(match_unordered_list_element("- test"), Err(Error(("", ErrorKind::Tag))));
			assert_eq!(match_unordered_list_element("-"), Err(Error(("", ErrorKind::Tag))));
		}

		#[test]
		fn test_match_unordered_list() {
			assert_eq!(match_unordered_list("- this is an element"), Err(Error(("", ErrorKind::Tag))));
			assert_eq!(match_unordered_list("- this is an element\n"), Ok(("", vec![vec![MarkdownInline::Plaintext(String::from("this is an element"))]])));
			assert_eq!(match_unordered_list("- this is an element\n- here is another\n"), Ok(("", vec![vec![MarkdownInline::Plaintext(String::from("this is an element"))], vec![MarkdownInline::Plaintext(String::from("here is another"))]])));
		}

		#[test]
		fn test_match_ordered_list_tag() {
			assert_eq!(match_ordered_list_tag("1. "), Ok(("", "1")));
			assert_eq!(match_ordered_list_tag("1234567. "), Ok(("", "1234567")));
			assert_eq!(match_ordered_list_tag("3. and some more"), Ok(("and some more", "3")));
			assert_eq!(match_ordered_list_tag("1"), Err(Error(("", ErrorKind::Tag))));
			assert_eq!(match_ordered_list_tag("1.and some more"), Err(Error(("and some more", ErrorKind::Tag))));
			assert_eq!(match_ordered_list_tag("1111."), Err(Error(("", ErrorKind::Tag))));
			assert_eq!(match_ordered_list_tag(""), Err(Error(("", ErrorKind::TakeWhile1))));
		}

		#[test]
		fn test_match_ordered_list_element() {
			assert_eq!(match_ordered_list_element("1. this is an element\n"), Ok(("", vec![MarkdownInline::Plaintext(String::from("this is an element"))])));
			assert_eq!(match_ordered_list_element("1. this is an element\n1. here is another\n"), Ok(("1. here is another\n", vec![MarkdownInline::Plaintext(String::from("this is an element"))])));
			assert_eq!(match_ordered_list_element(""), Err(Error(("", ErrorKind::TakeWhile1))));
			assert_eq!(match_ordered_list_element(""), Err(Error(("", ErrorKind::TakeWhile1))));
			assert_eq!(match_ordered_list_element("1. \n"), Ok(("", vec![])));
			assert_eq!(match_ordered_list_element("1. test"), Err(Error(("", ErrorKind::Tag))));
			assert_eq!(match_ordered_list_element("1. "), Err(Error(("", ErrorKind::Tag))));
			assert_eq!(match_ordered_list_element("1."), Err(Error(("", ErrorKind::Tag))));
		}

		#[test]
		fn test_match_ordered_list() {
			assert_eq!(match_ordered_list("1. this is an element\n"), Ok(("", vec![vec![MarkdownInline::Plaintext(String::from("this is an element"))]])));
			assert_eq!(match_ordered_list("1. test"), Err(Error(("", ErrorKind::Tag))));
			assert_eq!(match_ordered_list("1. this is an element\n2. here is another\n"), Ok(("", vec![vec!(MarkdownInline::Plaintext(String::from("this is an element"))), vec![MarkdownInline::Plaintext(String::from("here is another"))]])));
		}

		fn test_match_markdown() {
			assert_eq!(
				match_markdown("# Foobar\n\nFoobar is a Python library for dealing with word pluralization.\n\n## Installation\n\nUse the package manager [pip](https://pip.pypa.io/en/stable/) to install foobar.\n"),
				Ok(("", vec![
					Markdown::Heading(1, vec![MarkdownInline::Plaintext(String::from("Foobar"))]),
					Markdown::Line(vec![]),
					Markdown::Line(vec![MarkdownInline::Plaintext(String::from("Foobar is a Python library for dealing with word pluralization."))]),
					Markdown::Line(vec![]),
					Markdown::Heading(2, vec![MarkdownInline::Plaintext(String::from("Installation"))]),
					Markdown::Line(vec![]),
					Markdown::Line(vec![
						MarkdownInline::Plaintext(String::from("Use the package manager ")),
						MarkdownInline::Link(String::from("pip"), String::from("https://pip.pypa.io/en/stable/")),
						MarkdownInline::Plaintext(String::from(" to install foobar.")),
					]),
				]))
			)
		}
    }
}
