open Base
open Result
open AST
open Util

let ( ||> ) f g x = f (g x)

let fail_at_exn l e = Result.fail (tagged_exn l e)

let fail_at l msg = fail_at_exn (Some l) (RuntimeError msg)

let fail msg = fail_at_exn None (RuntimeError msg)

let return_single x = return [x]

let truthy = function
  | Nil -> false
  | Bool b -> b
  | _ -> true

let eq = function
  | String a, String b -> String.equal a b
  | Int a, Int b -> Z.equal a b
  | Float a, Float b -> Float.equal a b
  | Bool a, Bool b -> Bool.equal a b
  | Nil, Nil -> true
  | _ -> false

let arith_str span (fs, fi, ff) a' b' =
  match (a', b') with
  | String a, String b -> return @@ fs (a, b)
  | Int a, Int b -> return @@ fi (a, b)
  | Float a, Float b -> return @@ ff (a, b)
  | _ -> fail_at span "invalid arguments to arithmetic operator"

(* TODO let caller specify no-match 
 * truthy values: nil is false, all others are true
 * string * int
 * float ** int
 * obj op obj  (python style)
 *)
let arith span opi opf a' b' =
  try
    match (a', b') with
    | Int a, Int b -> return @@ Int (opi a b)
    | Float a, Float b -> return @@ Float (opf a b)
    | _ -> fail_at span "invalid arguments to arithmetic operator"
  with e -> fail_at_exn (Some span) e

let cmp op span =
  arith_str span
    ( bool ||> op ||> uncurry String.compare
    , bool ||> op ||> uncurry Z.compare
    , bool ||> op ||> uncurry Float.compare )

let to_lox_string = function
  | Int x -> return @@ Z.to_string x
  | Float x -> return @@ Float.to_string x
  | Bool x -> return @@ Bool.to_string x
  | String x -> return x
  | Nil -> return "nil"

let rec eval_expr env expr =
  let span = expr.tag in
  match expr.expr with
  | Primary p -> eval_expr env p
  | This -> fail_at span "this is only allowed in methods"
  | ID s -> 
      begin match Hashtbl.find env.vars s with
      | None -> fail_at span @@ "unknown variable " ^ s
      | Some p -> return p (* eval_expr env e *)
      end
  | Assign (i, e) ->
      begin match Hashtbl.find env.vars i with
      | None -> fail_at span @@ "cannot assign to unknown variable " ^ i
      | Some _ -> eval_expr env e >>= fun p ->
          Hashtbl.set env.vars ~key:i ~data:p;
          return Nil
      end

  | Dot p -> eval_expr env (List.hd_exn p)
  | AssignTo _ (* (o, i, e) *) -> fail_at span "OOP ... TODO"
  | SuperID _ (* (o, i, e) *) -> fail_at span "OOP ... TODO"
  | Call _ (* (f, fs) *) -> fail_at span "TODO function"
  | Primitive p -> return p
  | BinaryOp (op, a', b') -> begin
      eval_expr env a'
      >>= fun a ->
      eval_expr env b'
      >>= fun b ->
      match op with
      | Eq -> return @@ bool @@ eq (a, b)
      | Ne -> return @@ bool @@ not @@ eq (a, b)
      | And -> begin match a, b with
          | Bool ba, Bool bb -> return @@ bool @@ (ba && bb)
          | _, _ -> fail "cannot find logical conjunction" 
      end
      | Or -> begin match a, b with
          | Bool ba, Bool bb -> return @@ bool @@ (ba || bb)
          | _, _ -> fail "cannot find logical disjunction" 
      end
      | Gt -> cmp (( < ) 0) span a b
      | Ge -> cmp (( <= ) 0) span a b
      | Lt -> cmp (( > ) 0) span a b
      | Le -> cmp (( >= ) 0) span a b
      | Sub -> arith span Z.( - ) Float.( - ) a b
      | Add ->
        begin match (a, b) with
        | String s, x -> to_lox_string x >>= fun str_x -> return @@ String (s ^ str_x)
        | x, String s -> to_lox_string x >>= fun str_x -> return @@ String (str_x ^ s)
        | a, b -> arith span Z.( + ) Float.( + ) a b 
        end
      | Mul -> arith span Z.( * ) Float.( * ) a b
      | Div -> arith span Z.( / ) Float.( / ) a b
      | Exp -> begin match (a, b) with
        | Float a, Float b -> return @@ float @@ Float.(a ** b)
        | Float a, Int b   -> return @@ float @@ Float.(int_pow a (Z.to_int b))
        | Int a, Int b     -> return @@ int @@ Z.(a ** (to_int b))
        | _, _ -> fail_at span "can't exp"
      end
  end
  | UnaryOp (op, e') -> begin
      eval_expr env e'
      >>= fun e ->
      match op with
      | Negation -> begin match e with
        | Int z -> return @@ Int (Z.neg z)
        | Float f -> return @@ Float (-.f)
        | prim -> fail_at span ("can't negate primitive" ^ show_primitive prim)
      end
      | Complement -> begin match e with
        | Int z -> return @@ Int (Z.( ~! ) z)
        | Bool b -> return @@ Bool (not b)
        | prim -> fail_at span ("can't find binary complement of primitive" ^ show_primitive prim)
      end
    end

and eval_stmt env stmt = 
  match stmt.stmt with
  | Expr e -> eval_expr env e >>= return_single
  | For (_, _, _, body) -> eval_stmt env body
  (* TODO use perfect hash for globals and other names known entirely at compile-time *)
  | Var (name, e) ->
      eval_expr env e >>= fun p -> Hashtbl.set env.vars ~key:name ~data:p;
      return_single Nil
  | Magic ("ast", Some (st)) | Magic("parse", Some(st)) ->
      Stdio.Out_channel.fprintf env.channel "%s\n" @@ pretty_stmt st.stmt;
      return_single Nil
  | Magic (name, _) -> return_single @@ string @@ "unknown magic " ^ name
  | If (cond', a, b) -> 
      eval_expr env cond' >>= fun cond ->
      (if truthy cond 
      then eval_stmt env a 
      else Option.value_map ~default:(return_single Nil) ~f:(eval_stmt env) b)
  | Print e -> 
      eval_expr env e >>= fun p -> 
      to_lox_string p >>= fun str ->
      Stdio.Out_channel.fprintf env.channel "%s\n" str;
      return_single Nil
  | While (e, _) -> eval_expr env e >>= return_single
  | Block ss ->
      let go l s = match l with
        | Ok [] -> eval_stmt env s
        | Ok l  -> eval_stmt env s >>= fun h -> return @@ h @ l
        | e -> e
      in List.fold ~init:(return []) ~f:go ss >>= fun ps -> return @@ List.rev ps

  | Return _ -> fail "nope" 

let init_vars () = Hashtbl.create ~size:100 (module String)

let eval_with env ast = 
  eval_stmt env ast

let eval ast = 
  eval_with { channel=Stdio.stdout; vars=init_vars () } ast
