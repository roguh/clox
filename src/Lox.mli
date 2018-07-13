open AST

val runFile : Stdio.Out_channel.t -> string -> unit
val repl_init_state : Stdio.Out_channel.t -> env
val rep : Stdio.Out_channel.t -> env -> string -> unit
(*
val repl_basic : Stdio.Out_channel.t -> unit
*)
