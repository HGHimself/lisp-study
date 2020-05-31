// lib.rs

/// Type-erased errors.
pub type BoxError = std::boxed::Box<dyn
	std::error::Error   // must implement Error to satisfy ?
	+ std::marker::Send // needed for threads
	+ std::marker::Sync // needed for threads
>;

#[derive(Clone,Debug,PartialEq)]
pub enum Markdown {
    Heading(usize, String),
    Bold(String),
    Italic(String),
    OrderedList(Vec<String>),
    UnorderedList(Vec<String>),
    Code(String),
    Link(String, String),
    Image(String, String),
}

pub(self) mod parsers {
    use super::Markdown;

    fn not_whitespace(i: &str) -> nom::IResult<&str, &str> {
        nom::bytes::complete::is_not(" \t")(i)
    }

	fn match_heading(i: &str) -> nom::IResult<&str, &str>  {
		nom::bytes::complete::take_while1(|c| c == '#')(i)
	}

    fn match_header_tag(i: &str) -> nom::IResult<&str, usize> {
        nom::combinator::map( match_heading, |s: &str| s.len() )(i)
    }

	fn match_header(i: &str) -> nom::IResult<&str, Markdown> {
		nom::combinator::map(
			nom::sequence::tuple((
				match_header_tag,
				match_line
			)),
			|r| Markdown::Heading(r.0, r.1.to_string())
		)(i)
	}

    fn match_line(i: &str) -> nom::IResult<&str, &str> {
        nom::bytes::complete::take_while1(|c| c != '\n')(i)
    }

    #[cfg(test)]
    mod tests {
        use super::*;

        #[test]
        fn test_non_whitespace() {
            assert_eq!(not_whitespace("abcd efg"), Ok((" efg", "abcd")));
            assert_eq!(not_whitespace("ab\tcd efg"), Ok(("\tcd efg", "ab")));
            assert_eq!(not_whitespace("abcd efg"), Ok((" efg", "abcd")));
			assert_eq!(not_whitespace("abcd\tefg"), Ok(("\tefg", "abcd")));
			assert_eq!(not_whitespace(" abcdefg"), Err(nom::Err::Error((" abcdefg", nom::error::ErrorKind::IsNot))));
        }

        #[test]
        fn test_match_headers() {
			assert_eq!(match_heading("#"), Ok(("", "#")));
            assert_eq!(match_header_tag("#"), Ok(("", 1)));
			assert_eq!(match_heading("###"), Ok(("", "###")));
            assert_eq!(match_header_tag("###"), Ok(("", 3)));
			assert_eq!(match_heading("# h1"), Ok((" h1", "#")));
            assert_eq!(match_header_tag("# h1"), Ok((" h1", 1)));
			assert_eq!(match_heading(" "), Err(nom::Err::Error((" ", nom::error::ErrorKind::TakeWhile1))));
			assert_eq!(match_header_tag(" "), Err(nom::Err::Error((" ", nom::error::ErrorKind::TakeWhile1))));
        }

        #[test]
        fn test_match_line() {
            assert_eq!(match_line("and then afterwards we were able to see everything\n"), Ok(("\n", "and then afterwards we were able to see everything")));
            assert_eq!(match_line("but\nthen later"), Ok(("\nthen later", "but")));
            assert_eq!(match_line("okay\n\n"), Ok(("\n\n", "okay")));
			assert_eq!(match_line("\n"), Err(nom::Err::Error(("\n", nom::error::ErrorKind::TakeWhile1))));
        }

		#[test]
		fn test_match_header() {
			assert_eq!(match_header("# h1"), Ok(("", Markdown::Heading(1, String::from(" h1")))));
		}
    }
}
