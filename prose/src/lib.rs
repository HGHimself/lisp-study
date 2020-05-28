// lib.rs

/// Type-erased errors.
pub type BoxError = std::boxed::Box<dyn
	std::error::Error   // must implement Error to satisfy ?
	+ std::marker::Send // needed for threads
	+ std::marker::Sync // needed for threads
>;

#[derive(Clone,Debug)]
pub enum Markdown {
    Heading(u8, std::string::String),
    Bold(std::string::String),
    Italic(std::string::String),
    OrderedList(std::vec::Vec<std::string::String>),
    UnorderedList(std::vec::Vec<std::string::String>),
    Code(std::string::String),
    Link(std::string::String, std::string::String),
    Image(std::string::String, std::string::String),
}

pub(self) mod parsers {
    use super::Markdown;

    fn not_whitespace(i: &str) -> nom::IResult<&str, &str> {
        nom::bytes::complete::is_not(" \t")(i)
    }

    fn match_heading(i: &str) -> nom::IResult<&str, &str> {
        nom::bytes::complete::take_while1(|c| c == '#')(i)
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
        fn test_match_heading() {
            assert_eq!(match_heading("#"), Ok(("", "#")));
            assert_eq!(match_heading("###"), Ok(("", "###")));
            assert_eq!(match_heading("# h1"), Ok((" h1", "#")));
			assert_eq!(match_heading(" "), Err(nom::Err::Error((" ", nom::error::ErrorKind::TakeWhile1))));
        }

        #[test]
        fn test_match_line() {
            assert_eq!(match_line("and then afterwards we were able to see everything\n"), Ok(("\n", "and then afterwards we were able to see everything")));
            assert_eq!(match_line("but\nthen later"), Ok(("\nthen later", "but")));
            assert_eq!(match_line("okay\n\n"), Ok(("\n\n", "okay")));
			assert_eq!(match_line("\n"), Err(nom::Err::Error(("\n", nom::error::ErrorKind::TakeWhile1))));
        }
    }
}
