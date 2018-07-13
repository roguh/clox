open Base
open Result
open AST

let fail m = Error (m, None)

let fail_at l m = Error (m, Some l)

let mk_tag t e = {tag = t; expr = e}

let merge_span {start = i1; _} {stop = f2; _} = {start = i1; stop = f2}

let mk_tag_from s1 s2 t = mk_tag (merge_span s1 s2) t

let prim t p = mk_tag t (Primitive p)

let pretty_primitive = function
  | Int z -> Z.to_string z
  | Float f -> Float.to_string f
  | Bool b -> Bool.to_string b
  | String s -> String.escaped s
  | Nil -> "< nil >"

let pretty_expr e = show_expr 
  (fun fmt sp -> Caml.Format.fprintf fmt "%s" @@ show_span sp) e

let pretty_stmt s = show_stmt
  (fun fmt sp -> Caml.Format.fprintf fmt "%s" @@ show_span sp) s

let id x = x

let uncurry f (a, b) = f a b
