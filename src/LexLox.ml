include Tokens

open Base
open LexBuffer
open AST

(* use custom lexbuffer to keep track of source location *)
module Sedlexing = LexBuffer

(** Signals a lexing error at the provided source location.  *)
exception LexErrorAt of (AST.position * string)

(** Signals a parsing error at the provided token and its start and end locations. *)
exception ParseErrorAt of (token * AST.position * AST.position)

(* Register exceptions for pretty printing *)
(*
let _ =
  let open Location in
  register_error_of_exn (function
    | LexError (pos, msg) ->
      let loc = { loc_start = pos; loc_end = pos; loc_ghost = false} in
      Some { loc; msg; sub=[]; if_highlight=""; }
    | ParseError (token, loc_start, loc_end) ->
      let loc = Location.{ loc_start; loc_end; loc_ghost = false} in
      let msg =
        show_token token
        |> Printf.sprintf "parse error while reading token '%s'" in
      Some { loc; msg; sub=[]; if_highlight=""; }
    | _ -> None)
*)


let failwith {pos;_} s = raise (LexErrorAt (parse_pos pos, s))

let illegal buf c =
  Char.escaped c
  |> Printf.sprintf "unexpected character in expression: '%s'"
  |> failwith buf

(* regular expressions  *)
let blank = [%sedlex.regexp? Chars " \t"] 
let newline = [%sedlex.regexp? '\r' | '\n' | "\r\n" ]

(* swallows whitespace and comments *)
let rec garbage buf =
  match%sedlex buf with
  | Plus (newline | blank) -> garbage buf
  | "/*" -> comment 1 buf
  | _ -> ()

(* allow nested comments, like OCaml *)
and comment depth buf =
  if depth = 0 then garbage buf else
  match%sedlex buf with
  | eof -> failwith buf "Unterminated comment at EOF" 
  | "/*" -> comment (depth + 1) buf
  | "*/" -> comment (depth - 1) buf
  | any -> comment depth buf
  | _ -> assert false

  (* TODO: BUG ___ is an integer :( *)
let digit   = [%sedlex.regexp? '0'..'9']
let digit_  = [%sedlex.regexp? digit | '_' ]
let hdigit  = [%sedlex.regexp? digit | 'A'..'F' | 'a'..'f']
let hexinit = [%sedlex.regexp? "0x" | "0X"]

let lexint   = [%sedlex.regexp? 
  (digit, Star digit_) |
  (hexinit | "0o" | "0O", hdigit, Star (hdigit | "_"))]
let lexintb  = [%sedlex.regexp? ("0b" | "0B"), Chars "01", Star (Chars "01_")]
let lexfloat = [%sedlex.regexp? 
  ('.', digit, Star digit_) | 
  (digit, Star digit_, '.', Star digit_)]

let letter   = [%sedlex.regexp? 'A'..'Z' | 'a'..'z']
(* TODO support unicode identifiers *)
let lexid    = [%sedlex.regexp? (letter | '_'), Star (letter | digit | '_')]

let lexstring = [%sedlex.regexp? '"', Star (alphabetic | Chars " _"), '"']

let get_lexeme = Sedlexing.Latin1.lexeme

(* "_" becomes "0" *)
let cleannum s = String.filter ~f:(fun c -> not @@ Char.equal '_' c) s

(* returns the next token *)
let token buf =
  garbage buf;
  match%sedlex buf with
  | eof  -> EOF
  
  | lexfloat  -> FLOAT (Float.of_string @@ cleannum @@ get_lexeme buf.buf)
  | lexint    -> INT   (None,   cleannum @@ get_lexeme buf.buf)
  | lexintb   -> INT   (Some 2, cleannum @@ get_lexeme buf.buf)
  | lexstring -> STRING (get_lexeme buf.buf)
  | "true"    -> TRUE
  | "false"   -> FALSE
  | "null"    -> NULL
  
  | '('  -> LPAREN
  | ')'  -> RPAREN
  | '{'  -> LBRACE
  | '}'  -> RBRACE
  | ','  -> COMMA
  | '.'  -> DOT
  | ';'  -> SEMICOLON
  | ':'  -> COLON
  | '-'  -> MINUS
  | '+'  -> PLUS
  | '*'  -> STAR
  | '/'  -> SLASH
  | "**" -> POW
  | "!=" -> BANGEQUAL
  | '!'  -> BANG
  | "==" -> EQUALEQUAL
  | '='  -> EQUAL
  | ">=" -> GREATEREQUAL
  | '>'  -> GREATER
  | "<=" -> LESSEQUAL
  | '<'  -> LESS
  
  | "and"    -> AND
  | "or"     -> OR
  | "class"  -> CLASS
  | "else"   -> ELSE
  | "fun"    -> FUN
  | "for"    -> FOR
  | "if"     -> IF
  | "print"  -> PRINT
  | "return" -> RETURN
  | "super"  -> SUPER
  | "this"   -> THIS
  | "var"    -> VAR
  | "while"  -> WHILE

  | lexid     -> ID (get_lexeme buf.buf)

  | _ -> illegal buf (Option.value ~default:' ' (Option.map ~f:Uchar.to_char_exn (next buf)))

(* wrapper around `token` that records start and end locations *)
let loc_token buf =
  let () = garbage buf in (* dispose of garbage before recording start location *)
  let loc_start = LexBuffer.next_loc buf in
  let t = token buf in
  let loc_end = LexBuffer.next_loc buf in
  (t, loc_start, loc_end)


(* menhir interface *)
type ('token, 'a) parser = ('token, 'a) MenhirLib.Convert.traditional

let parse buf p =
  let last_token = ref Lexing.(EOF, dummy_pos, dummy_pos) in
  let next_token () = last_token := loc_token buf; !last_token in
  try Ok(MenhirLib.Convert.Simplified.traditional2revised p next_token) with
  | LexErrorAt (pos, s)      -> Error(tagged_exn_pos pos @@ LexError s)
  | ParseErrorAt (tok, s, e) -> Error(tagged_exn_span s e @@ ParseError (show_token tok))
  | e -> let (_, start, stop) = !last_token in
         Error(tagged_exn_span (parse_pos start) (parse_pos stop) e)

let parse_string ?pos s p =
  parse (LexBuffer.of_ascii_string ?pos s) p

let parse_file ~file p =
  parse (LexBuffer.of_ascii_file file) p
