(* TODO hacky trick: add a semicolon to every repl input? 
       * alternative: define grammar so that last statement needs no ;
       *)
open Stdio
open Base
open Result
open AST
open Util
open ParseLox
open EvalLox

let fprintf = Out_channel.fprintf

let print_result channel r = 
  fprintf channel "%s\n" @@ Option.value_map ~default:"" ~f:pretty_primitive @@ List.last r

let print_error channel e =
  fprintf channel "%s\n" @@ show_tagged_exn e

let interpretFile channel f =
  (*
  (try Ok (In_channel.create f) with e -> Error (tagged_exn None e))
  >>= fun chn ->
  *)
  ast_of_file f >>= eval >>= fun p -> return @@ print_result channel p

let runFile channel f =
  (* print any errors encountered *) 
  Result.iter_error (interpretFile channel f) ~f:(print_error channel)

  (*
let rec repl_basic channel =
  try
    fprintf channel ">>> " ;
    Out_channel.flush Stdio.stdout ;
    let line' = In_channel.input_line ?fix_win_eol:(Some true) Stdio.stdin in
    match line' with
    | None -> ()
    | Some line ->
        let result = lex_str line >>= parse >>= eval in
        ( fprintf channel "%s\n"
        @@ match result with Ok p -> pretty_primitive p | Error e -> show_tagged_exn e ) ;
        Out_channel.flush Stdio.stdout ; repl_basic channel
  with _ -> fprintf channel "bye\n"
  *)

let repl_init_state channel = { channel=channel; vars=EvalLox.init_vars () }

let rep channel state v =
  let result = ast_of_string v >>= eval_with state >>= fun p -> 
    return (match List.last p with Some Nil -> () | _ -> print_result channel p) in
  Result.iter_error result ~f:(print_error channel) ;
  Out_channel.flush Stdio.stdout

let%test_unit "empty file" = runFile Out_channel.stdout "tests/empty.lox"

(* let%test "be good" = false *)
