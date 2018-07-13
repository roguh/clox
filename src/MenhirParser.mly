%{
(* OCaml preamble *)
open AST
module List = Base.List

let mk_expr = AST.parse_expr 
let mk_stmt = AST.parse_stmt
%}

(* tokens *)
%token <int option * string> INT (* base and bigint. convert to bigint later *)
%token <float> FLOAT
%token <string> ID
%token <string> STRING
%token TRUE
%token FALSE
%token NULL
%token LPAREN
%token RPAREN
%token LBRACE
%token RBRACE
%token COMMA
%token DOT
%token SEMICOLON
%token COLON
%token BANG
%token EQUAL 
%token EQUALEQUAL BANGEQUAL AND OR LESS LESSEQUAL GREATER GREATEREQUAL 
%token PLUS MINUS STAR SLASH POW
%token CLASS
%token ELSE
%token FUN
%token FOR
%token IF
%token PRINT
%token RETURN
%token SUPER
%token THIS
%token VAR
%token WHILE
%token EOF


%left EQUAL
%left EQUALEQUAL
%left BANGEQUAL
%left AND
%left OR
%left LESS
%left LESSEQUAL
%left GREATER
%left GREATEREQUAL
%left PLUS
%left MINUS
%left STAR
%left SLASH
%left POW
%left BANG

%start <AST.ast> ast_eof

%%

ast_eof:
  | a = separated_list(SEMICOLON, stmt); EOF { mk_stmt $loc @@ Block a }
  ;


expr:
  | p = primary_                    { p }
  | e1 = expr; o = binop; e2 = expr { mk_expr $loc @@ o e1 e2 }
  | o = unaryop; e = expr           { mk_expr $loc @@ o e }
  | i = ID; EQUAL; e = expr         { mk_expr $loc @@ Assign (i, e) }
  | c = primary; DOT; i = ID; EQUAL; e = expr
                                    { mk_expr $loc @@ AssignTo (c, i, e) }
  | c = primary; a = nonempty_list(call); EQUAL; e = expr { 
      let ar = List.rev a in
      let h, t = List.hd_exn ar, List.rev @@ List.tl_exn ar
      in mk_expr $loc @@ AssignTo (
         (if List.length t > 0 then mk_expr $loc @@ Call (c, t) else c), 
         (match h with 
           | Accessor i -> i 
           | _ -> raise @@ ParseError "assignment expected l-value, not function call"),
         e) }
  ;

primary_:
  | p = primary { p }
  | p = primary; a = nonempty_list(call)
                                       { mk_expr $loc @@ Call (p, a) }
  ;

primary:
  | i = ID     { mk_expr $loc @@ ID i }
  | THIS       { mk_expr $loc @@ This }
  | NULL       { mk_expr $loc @@ Primitive Nil }
  | TRUE       { mk_expr $loc @@ Primitive (Bool true) }
  | FALSE      { mk_expr $loc @@ Primitive (Bool false) }
  | f = FLOAT  { mk_expr $loc @@ Primitive (Float f) }
  | s = STRING { mk_expr $loc @@ Primitive (String s) }
  | bi = INT   { mk_expr $loc @@ let (base', i) = bi in Primitive (
                 Int (Base.Option.value_map base'
                   ~default:(Z.of_string i)
                   ~f:(fun base -> Z.of_string_base base i))) }
  | SUPER; DOT; i = ID       { mk_expr $loc @@ SuperID i }
  | LPAREN; p = expr; RPAREN { mk_expr $loc @@ Primary p }
  ;

call:
  | LPAREN; a = separated_list(COMMA, expr); RPAREN { Args a }
  | DOT; i = ID                                     { Accessor i }

%inline binop:
 | EQUALEQUAL   { binaryop Eq }
 | BANGEQUAL    { binaryop Ne }
 | AND          { binaryop And }
 | OR           { binaryop Or }
 | LESS         { binaryop Lt }
 | LESSEQUAL    { binaryop Le }
 | GREATER      { binaryop Gt }
 | GREATEREQUAL { binaryop Ge }
 | POW          { binaryop Exp }
 | STAR         { binaryop Mul }
 | SLASH        { binaryop Div }
 | PLUS         { binaryop Add }
 | MINUS        { binaryop Sub }
 ;

%inline unaryop:
 | MINUS { unaryop Negation }
 | BANG  { unaryop Complement }
 ;

stmt:
 | e = expr { mk_stmt $loc @@ Expr e }; 
 | VAR; i = ID; EQUAL; e = expr { mk_stmt $loc @@ Var (i, e) }
 | PRINT; e = expr           { mk_stmt $loc @@ Print e }
 | RETURN; e = expr          { mk_stmt $loc @@ Return (Some e) }
 | RETURN;                   { mk_stmt $loc @@ Return (None) }
 | LBRACE; ss = separated_list(SEMICOLON, stmt); RBRACE; 
    { mk_stmt $loc @@ Block ss }
 | WHILE; LPAREN; e = expr; RPAREN; s = stmt;
    { mk_stmt $loc @@ While (e, s) }
 (* TODO explicitly resolve shift-reduce conflict with dangling-else *)
 | IF; LPAREN; cond = expr; RPAREN; e = stmt; 
    { mk_stmt $loc @@ If (cond, e, None) }
 | IF; LPAREN; cond = expr; RPAREN; e1 = stmt; ELSE; e2 = stmt; 
    { mk_stmt $loc @@ If (cond, e1, Some e2) }
 | COLON; i = ID; s = stmt
    { mk_stmt $loc @@ Magic (i, Some s) }
 | COLON; i = ID
    { mk_stmt $loc @@ Magic (i, None) }
 ;
%%
