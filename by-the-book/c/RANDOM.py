import string
import random

out = "constants arithmetic strings".split()

num_ops = list("+-*/%^&|*") + ["**"]

raw = lambda it: it

identifier = ('identifier', raw, lambda: random.choice(string.ascii_letters + '_') + "".join(random.choices(string.ascii_letters + '_' + string.digits, k=random.choice([1, 5, 128]))))

numbers = [

('0.0-1.0', float, lambda: random.random()),
# TODO support sci
# ('0.0-1e250', float, lambda: 1e250 * random.random()),
('0.0-1e10', float, lambda: 1e10 * random.random()),

('0-1000', int, lambda: random.randint(0, 1000)),
('-1000-1000', int, lambda: random.randint(-1000, 1000)),
('-1000000-1000000', int, lambda: random.randint(-1000000, 1000000)),
('-2**63-2**63', int, lambda: random.randint(-9223372036854775808, 9223372036854775808)),
]

strings = [
(identifier[0], repr, identifier[2]),
('1 char', repr, lambda: "".join(random.choices(string.printable, k=1))),
('5 char', repr, lambda: "".join(random.choices(string.printable, k=5))),
# ('5000 char', repr, lambda: "".join(random.choices(string.printable, k=5000))),
('only quotes', repr, lambda: "".join(random.choices(["'", '"'], k=10))),
]

pickers = [
('nil', raw, lambda: 'nil'),
('bool', raw, lambda: random.choice("true false".split())),

('0x 0-1000', hex, lambda: random.randint(0, 1000)),
('0x -1000-1000', hex, lambda: random.randint(-1000, 1000)),
('0x -1000000-1000000', hex, lambda: random.randint(-1000000, 1000000)),
('0x -2**63-2**63', hex, lambda: random.randint(-9223372036854775808, 9223372036854775808)),

] + strings + numbers

collections = [
('list 1'),
('list 10-100'),
('hashmap 1'),
('hashmap 10-100'),
]

nested_collections = [
('list with lists'),
('hashmap with hashmap'),
('hashmaps and lists and more'),
]

if "constants" in out:
    for (name, converter, creator) in pickers:
        print(f"// {name}",
              "\n".join(
                f"print({converter(creator())});"
                for _ in range(20)
            ),
            "\n\n\n",
            sep="\n")


if "arithmetic" in out:
    for op1 in num_ops:
        for op2 in num_ops:
            eq = f"a{op1}b{op2}c"
            print("//", eq)
            for (_, sa, ca) in numbers:
                for (_, sb, cb) in numbers:
                    for (_, sc, cc) in numbers:
                        _a = lambda: sa(ca())
                        _b = lambda: sb(cb())
                        _c = lambda: sc(cc())
                        for _ in range(10):
                            a, b, c = _a(), _b(), _c()
                            # expect = eval(eq)
                            print(
                                # print({expect}=={eq});
                                f"{{print(__line__);var a={a};var b={b};var c={c};print({eq});}}"
                            )

if "strings" in out:
    print()
