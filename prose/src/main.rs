// main.rs

extern crate prose;
use prose::BoxError;

fn main() -> std::result::Result<(), BoxError> {
	// Inside the body of main we can now use the ? operator.
	prose::markdown()?;
	Ok(())
}
