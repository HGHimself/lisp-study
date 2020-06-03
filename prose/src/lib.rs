// lib.rs

/// Type-erased errors.
pub type BoxError = std::boxed::Box<dyn
	std::error::Error   // must implement Error to satisfy ?
	+ std::marker::Send // needed for threads
	+ std::marker::Sync // needed for threads
>;

#[derive(Clone,Debug,PartialEq)]
pub enum Markdown {
	// some markdown has a nested structure
	// for example, you can have bold text inside of an ordered list
    Heading(usize, MarkdownText),
	OrderedList(Vec<MarkdownText>),
    UnorderedList(Vec<MarkdownText>),
	Line(MarkdownText),
	// some markdown is terminal
	// we will have to decide if the you can have nested inline tags
	// but for now we will simplify
    Codeblock(String),
    Link(String, String),
    Image(String, String),
	InlineCode(String),
	Bold(String),
    Italic(String),
	Plaintext(String),
}

#[derive(Clone,Debug,PartialEq)]
pub struct MarkdownText(Vec<Markdown>);


pub(self) mod parsers {
    use super::Markdown;

	fn match_boldtext(i: &str) -> nom::IResult<&str, Markdown> {
		nom::combinator::map(
			nom::sequence::delimited(
				nom::bytes::complete::tag("**"),
				nom::bytes::complete::is_not("**"),
				nom::bytes::complete::tag("**"),
			),
			|s: &str| Markdown::Bold(s.to_string())
		)(i)
	}

	fn match_italics(i: &str) -> nom::IResult<&str, Markdown> {
		nom::combinator::map(
			nom::sequence::delimited(
				nom::bytes::complete::tag("*"),
				nom::bytes::complete::is_not("*"),
				nom::bytes::complete::tag("*"),
			),
			|s: &str| Markdown::Italic(s.to_string())
		)(i)
	}

	fn match_inline_code(i: &str) -> nom::IResult<&str, Markdown> {
		nom::combinator::map(
			nom::sequence::delimited(
				nom::bytes::complete::tag("`"),
				nom::bytes::complete::is_not("`"),
				nom::bytes::complete::tag("`"),
			),
			|s: &str| Markdown::InlineCode(s.to_string())
		)(i)
	}

	fn match_link(i: &str) -> nom::IResult<&str, Markdown> {
		nom::combinator::map(
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
			),
			|(tag, link): (&str, &str)| Markdown::Link(tag.to_string(), link.to_string())
		)(i)
	}

	fn match_image(i: &str) -> nom::IResult<&str, Markdown> {
		nom::combinator::map(
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
			),
			|(tag, link): (&str, &str)| Markdown::Image(tag.to_string(), link.to_string())
		)(i)
	}

    #[cfg(test)]
    mod tests {
        use super::*;

		#[test]
		fn test_match_italics() {
			assert_eq!(match_italics("*here is italic*"), Ok(("", Markdown::Italic(String::from("here is italic")))));
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
			assert_eq!(match_boldtext("**here is bold**"), Ok(("", Markdown::Bold(String::from("here is bold")))));
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
			assert_eq!(match_inline_code("`here is code`"), Ok(("", Markdown::InlineCode(String::from("here is code")))));
			assert_eq!(match_inline_code("`here is code"), Err(nom::Err::Error(("", nom::error::ErrorKind::Tag))));
			assert_eq!(match_inline_code("here is code`"), Err(nom::Err::Error(("here is code`", nom::error::ErrorKind::Tag))));
			assert_eq!(match_inline_code("``"), Err(nom::Err::Error(("`", nom::error::ErrorKind::IsNot))));
			assert_eq!(match_inline_code("`"), Err(nom::Err::Error(("", nom::error::ErrorKind::IsNot))));
			assert_eq!(match_inline_code(""), Err(nom::Err::Error(("", nom::error::ErrorKind::Tag))));
		}

		#[test]
		fn test_match_link() {
			assert_eq!(match_link("[title](https://www.example.com)"), Ok(("", (Markdown::Link(String::from("title"), String::from("https://www.example.com"))))));
		}

		fn test_match_image() {
			assert_eq!(match_link("	![alt text](image.jpg)"), Ok(("", (Markdown::Link(String::from("alt text"), String::from("image.jpg"))))));
		}
    }
}
