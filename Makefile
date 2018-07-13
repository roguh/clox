all:
	dune build src/cli.exe

test:
	dune runtest

clean:
	dune clean

format:
	ocamlformat --margin 100 --no-indicate-nested-or-patterns --ocp-indent-compat --field-space=loose --break-struct=natural --inplace src/{AST,Cli,CompileLox,EvalLox,LexBuffer,Lox,Parser,Util}.ml
