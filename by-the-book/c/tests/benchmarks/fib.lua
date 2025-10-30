#!/usr/bin/env lua

function fib(n)
    if n < 2 then
        return n
    end
    return fib(n - 2) + fib(n - 1)
end

start = os.clock()
print(fib(35))
print(os.clock() - start)
