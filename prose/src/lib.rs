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

	// this will match a whole line
	fn match_line(i: &str) -> nom::IResult<&str, &str> {
        nom::bytes::complete::take_while1(|c| c != '\n')(i)
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
	fn match_header(i: &str) -> nom::IResult<&str, Markdown> {
		nom::combinator::map(
			nom::sequence::tuple(( match_header_tag, match_line )),
			|r| Markdown::Heading(r.0, r.1.to_string())
		)(i)
	}

	fn match_unordered_list_tag(i: &str) -> nom::IResult<&str, &str> {
		nom::sequence::terminated(
			nom::bytes::complete::tag("-"),
			nom::bytes::complete::tag(" ")
		)(i)
	}

	fn match_unordered_list_element(i: &str) -> nom::IResult<&str, &str> {
		nom::sequence::preceded(
			match_unordered_list_tag,
			match_line
		)(i)
	}

	fn match_unordered_list(i: &str)  -> nom::IResult<&str, Markdown> {
		nom::combinator::map(
			nom::multi::many0(match_unordered_list_element),
			|r| Markdown::UnorderedList(r.iter().map(|s| s.to_string()).collect())
		)(i)
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
        fn test_match_line() {
            assert_eq!(match_line("and then afterwards we were able to see everything\n"), Ok(("\n", "and then afterwards we were able to see everything")));
            assert_eq!(match_line("but\nthen later"), Ok(("\nthen later", "but")));
            assert_eq!(match_line("okay\n\n"), Ok(("\n\n", "okay")));
			assert_eq!(match_line("\n"), Err(nom::Err::Error(("\n", nom::error::ErrorKind::TakeWhile1))));
        }

        #[test]
        fn test_match_headers() {
			assert_eq!(match_header_tag("# "), Ok(("", 1)));
            assert_eq!(match_header_tag("### "), Ok(("", 3)));
            assert_eq!(match_header_tag("# h1"), Ok(("h1", 1)));
			assert_eq!(match_header_tag("# h1\n"), Ok(("h1\n", 1)));
			assert_eq!(match_header_tag(" "), Err(nom::Err::Error((" ", nom::error::ErrorKind::TakeWhile1))));
			assert_eq!(match_header_tag("#\n"), Err(nom::Err::Error(("\n", nom::error::ErrorKind::Tag))));
		}

		#[test]
		fn test_match_header() {
			assert_eq!(match_header("# h1"), Ok(("", Markdown::Heading(1, String::from("h1")))));
			assert_eq!(match_header("## h2"), Ok(("", Markdown::Heading(2, String::from("h2")))));
			assert_eq!(match_header("###  h3"), Ok(("", Markdown::Heading(3, String::from(" h3")))));
			assert_eq!(match_header("###h3"), Err(nom::Err::Error(("h3", nom::error::ErrorKind::Tag))));
			assert_eq!(match_header("###"), Err(nom::Err::Error(("", nom::error::ErrorKind::Tag))));
			assert_eq!(match_header(""), Err(nom::Err::Error(("", nom::error::ErrorKind::TakeWhile1))));
			assert_eq!(match_header("\n"), Err(nom::Err::Error(("\n", nom::error::ErrorKind::TakeWhile1))));
			assert_eq!(match_header("#\n"), Err(nom::Err::Error(("\n", nom::error::ErrorKind::Tag))));
			assert_eq!(match_header("# \n"), Err(nom::Err::Error(("\n", nom::error::ErrorKind::TakeWhile1))));
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
			assert_eq!(match_unordered_list_element("- this is an element"), Ok(("", "this is an element")));
			assert_eq!(match_unordered_list_element("- this is an element\n- here is another"), Ok(("\n- here is another", "this is an element")));
			assert_eq!(match_unordered_list_element(""), Err(nom::Err::Error(("", nom::error::ErrorKind::Tag))));
			assert_eq!(match_unordered_list_element("\n"), Err(nom::Err::Error(("\n", nom::error::ErrorKind::Tag))));
			assert_eq!(match_unordered_list_element("- \n"), Err(nom::Err::Error(("\n", nom::error::ErrorKind::TakeWhile1))));
			assert_eq!(match_unordered_list_element("-\n"), Err(nom::Err::Error(("\n", nom::error::ErrorKind::Tag))));
		}

		#[test]
		fn test_match_unordered_list() {
			assert_eq!(match_unordered_list("- this is an element"), Ok(("", Markdown::UnorderedList(vec![String::from("this is an element")]))));
			assert_eq!(match_unordered_list("- this is an element\n- here is another"), Ok(("", Markdown::UnorderedList(vec![String::from("this is an element"), String::from("here is another")]))));
		}
    }
}
