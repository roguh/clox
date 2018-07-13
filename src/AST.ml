type position = {line : int; col : int; fname : string option} 
  [@@deriving eq]

type primitive =
  | Nil
  | Bool of bool
  | Int of Z.t [@printer fun fmt z -> fprintf fmt "%sZ" (Z.to_string z)]
  | Float of float
  | String of string
  [@@deriving show {with_path = false}, eq]

type binaryop = Eq | Ne | And | Or | Lt | Le | Gt | Ge | Add | Sub | Mul | Div | Exp
  [@@deriving show {with_path = false}, eq]

type unaryop = Negation | Complement [@@deriving show {with_path = false}, eq]

type 'a decl_tagged = {tag : 'a; decl : 'a decl}

and 'a decl =
  (* lox extended: MORE GENERAL, doesn't require functions, can be any decl*)
  | Class of string * string option * 'a decl_tagged
  (* lox extended: MORE GENERAL, doesn't require a block, can be any stmt  *)
  | Func of string * string list * 'a stmt_tagged
  (*
  | Var of string * 'a expr_tagged
  *)
  | Stmt of 'a stmt_tagged

and 'a stmt_tagged = {tag : 'a; stmt : 'a stmt}

and 'a stmt =
  | Expr   of 'a expr_tagged
  (* TODO vardecl in for loop *)
  | For    of 'a expr_tagged option * 'a expr_tagged option * 'a expr_tagged option * 'a stmt_tagged
  | If     of 'a expr_tagged * 'a stmt_tagged * 'a stmt_tagged option
  | Magic  of string * 'a stmt_tagged option
  | Print  of 'a expr_tagged
  | Return of 'a expr_tagged option
  | While  of 'a expr_tagged * 'a stmt_tagged
  | Var    of string * 'a expr_tagged
  (*
  | Block  of 'a decl list
  *)
  | Block  of 'a stmt_tagged list
[@@deriving show {with_path = false}, eq]

and 'a expr_tagged = {tag : 'a; expr : 'a expr}

(* TODO same order as in MenhirParser.mly *)
and 'a expr =
  | BinaryOp   of binaryop * 'a expr_tagged * 'a expr_tagged
  | UnaryOp    of unaryop  * 'a expr_tagged
  | Primary    of 'a expr_tagged
  | Assign     of string * 'a expr_tagged
  | AssignTo   of 'a expr_tagged * string * 'a expr_tagged
  | ID         of string
  | Call       of 'a expr_tagged * ('a call_t list)
    (* object.i1.i2.i.3 *)
  | Dot        of 'a expr_tagged list
    (* super.i *)
  | SuperID    of string
  | This
  | Primitive  of primitive

and 'a call_t = 
  | Args of 'a expr_tagged list
  | Accessor of string
[@@deriving show {with_path = false}, eq]

open Base

exception LexError of string

exception ParseError of string

exception RuntimeError of string

type span = {start : position; stop : position}

type tagged_exn = {exn : exn; span : span option}

(* Printing functions *)

let show_position {line; col; _} = Int.to_string line ^ ":" ^ Int.to_string col

let show_span {start = i; stop = f} =
  Option.value_map i.fname ~default:"" ~f:(fun f -> "<" ^ f ^ "> ")
  ^ if equal_position i f then show_position i else show_position i ^ " to " ^ show_position f

let exn_to_string = function
  | LexError s -> Some "LexError", s
  | ParseError s -> Some "ParseError", s
  | RuntimeError s -> Some "RuntimeError", s
  | e -> None, Exn.to_string e

let show_tagged_exn {exn; span} =
  let (name, msg) = exn_to_string exn in 
  Option.value name ~default:"error"
  ^ Option.value_map span ~default:"" ~f:(fun span -> " at " ^ show_span span)
  ^ ": " ^ msg

(* Constructor functions *)

let tagged_exn_pos i e = {exn = e; span = Some {start = i; stop = i}}

let tagged_exn_span start stop e = {exn = e; span = Some {start; stop}}

let tagged_exn l e = {exn = e; span = l}

(* y u no haskelllll??? *)
let int a = Int a and float a = Float a and bool b = Bool b and string s = String s

let binaryop o e1 e2 = BinaryOp (o, e1, e2)

let unaryop o e = UnaryOp (o, e)

let parse_pos {Lexing.pos_lnum; Lexing.pos_cnum; Lexing.pos_fname; _} = {line =
  pos_lnum; col = pos_cnum; fname = if String.length pos_fname = 0 then None
  else Some pos_fname}

let parse_expr (start, stop) e = {expr = e; tag = {start = parse_pos start; stop = parse_pos stop}}

let parse_stmt (start, stop) e = {stmt = e; tag = {start = parse_pos start; stop = parse_pos stop}}

(* main type *)
type ast = span stmt_tagged

type env = { channel: Stdio.Out_channel.t; vars: (string, primitive) Hashtbl.t }

