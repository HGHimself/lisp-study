// lib.rs
use wasm_bindgen::prelude::*;
use web_sys::{console, Node};
use yew::virtual_dom::VNode;
use yew::{Component, ComponentLink, Html, InputData, ShouldRender};
use yew::prelude::*;
mod markdown;

struct Model {
    link: ComponentLink<Self>,
    markdown: String,
}

enum Msg {
    GotInput(String),
    Clicked,
}

impl Component for Model {
    type Message = Msg;
    type Properties = ();
    fn create(_: Self::Properties, link: ComponentLink<Self>) -> Self {
        Self {
            link,
            markdown: String::from("Not yet ready"),
        }
    }

    fn update(&mut self, msg: Self::Message) -> ShouldRender {
        match msg {
            Msg::GotInput(new_value) => {
                self.markdown = markdown::markdown::markdown(&new_value);
            }
            Msg::Clicked => {
                self.markdown = "blah blah blah".to_string();
            }
        }
        true
    }

    fn change(&mut self, _props: Self::Properties) -> ShouldRender {
        // Should only return "true" if new properties are different to
        // previously received properties.
        // This component has no properties so we will always return "false".
        false
    }

    fn view(&self) -> Html {
        let rendered_markdown = {
            let div = web_sys::window()
                .unwrap()
                .document()
                .unwrap()
                .create_element("div")
                .unwrap();
            div.set_inner_html(&self.markdown);
            console::log_1(&div);
            div
        };

        eprintln!("rendered_markdown: {:?}", rendered_markdown);
        let node = Node::from(rendered_markdown);
        let vnode = VNode::VRef(node);
        eprintln!("rendered_markdown: {:?}", vnode);

        html! {
            <div class={"main"}>
                <div>
                    <textarea rows=5
                        oninput=self.link.callback(|e: InputData| Msg::GotInput(e.value))
                        placeholder="placeholder">
                    </textarea>
                    <button onclick=self.link.callback(|_| Msg::Clicked)>{ "change value" }</button>
                </div>
                <div>
                    {&self.markdown}
                </div>
                <div>
                    {vnode}
                </div>
            </div>
        }

    }
}

#[wasm_bindgen(start)]
pub fn run_app() {
    App::<Model>::new().mount_to_body();
}
