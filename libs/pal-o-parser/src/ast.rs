use serde::{Serialize, Deserialize};
use std::fmt;

#[derive(Serialize, Deserialize, Debug, Clone)]
pub enum Expr {
    Lambda { bound: usize, body: Box<Expr> },
    Application { func: Box<Expr>, arg: Box<Expr> },
    Index(usize),
    Add(Box<Expr>, Box<Expr>),
    Sub(Box<Expr>, Box<Expr>),
    Monus(Box<Expr>, Box<Expr>),
    List(Vec<Expr>),
    FieldAccess { obj: Box<Expr>, field: String },
}

#[derive(Serialize, Deserialize, Debug)]
pub struct Program {
    pub body: Vec<Expr>,
}

impl Program {
    pub fn new(body: Vec<Expr>) -> Self {
        Self { body }
    }
}

impl fmt::Display for Expr {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            Expr::Index(n) => write!(f, "{}", n),
            Expr::Lambda { bound, body } => write!(f, "λ{}.{}", bound, body),
            Expr::Application { func, arg } => write!(f, "{} @ {}", func, arg),
            Expr::Add(left, right) => write!(f, "{} + {}", left, right),
            Expr::Sub(left, right) => write!(f, "{} - {}", left, right),
            Expr::Monus(left, right) => write!(f, "{} ∸ {}", left, right),
            Expr::List(elements) => {
                let elements_str = elements
                    .iter()
                    .map(|e| e.to_string())
                    .collect::<Vec<_>>()
                    .join(" ");
                write!(f, "[{}]", elements_str)
            }
            Expr::FieldAccess { obj, field } => write!(f, "{}.{}", obj, field),
        }
    }
}

impl fmt::Display for Program {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        let exprs = self.body.iter().map(|e| e.to_string()).collect::<Vec<_>>().join("\n");
        write!(f, "{}", exprs)
    }
}
