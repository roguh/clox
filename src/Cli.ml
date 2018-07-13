open Stdio
open Base
open Cmdliner
open Lox

let version = "v0.1"

let repl channel =
  (* LNoise.set_multiline true; *)
  (*
  LNoise.set_hints_callback (fun line ->
      if line <> "git remote add " then None
      else Some (" <this is the remote name> <this is the remote URL>",
                 LNoise.Yellow,
                 true)
    );
    *)
  (* LNoise.history_load ~filename:"history.txt" |> ignore; *)
  LNoise.history_set ~max_length:1000000 |> ignore ;
  (*
  LNoise.set_completion_callback begin fun line_so_far ln_completions ->
    if line_so_far <> "" && line_so_far.[0] = 'h' then
      ["Hey"; "Howard"; "Hughes";"Hocus"]
      |> List.iter (LNoise.add_completion ln_completions);
  end;
  *)
  let prompt = "lox> "
  and linenoise from_user =
    if Option.is_some @@ List.find ~f:(String.equal from_user) ["quit"; "exit"] 
    then false
    else ( LNoise.history_add from_user |> ignore ; true )
    (* LNoise.history_save ~filename:"history.txt" |> ignore; *)
  in
  let rec init_state = Lox.repl_init_state channel
  and user_input () =
    try
      match LNoise.linenoise prompt with
      | None -> ()
      | Some v -> if linenoise v then ( Lox.rep channel init_state v ; user_input () ) else ()
    with 
      | Caml.Sys.Break -> user_input ()
      | e -> printf "%s\n" (Exn.to_string e) 
  in
  user_input ()

let interpret msg =
  match msg with Some fname -> runFile Stdio.stdout fname | _ -> repl Stdio.stdout

let fname =
  let doc = "The file to interpret." in
  Arg.(value & pos 0 (some string) None & info [] ~docv:"FNAME" ~doc)

let term = Term.(const interpret $ fname)

let info =
  let doc = "interpret lox programs" in
  let man = [`S Manpage.s_bugs; `P "Report bugs to TODO"] in
  Term.info "lox" ~version ~doc ~exits:Term.default_exits ~man

let () = Term.exit @@ Term.eval (term, info)
