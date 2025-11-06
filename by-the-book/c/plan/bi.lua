collectgarbage("stop") -- testing only

-- todo use metatable...
-- setmetatable(t,({__pow=function(a,b)a[1]=a[1]+b;return a end}))
-- __add __sub __mul __div __mod __pow __eq __lt __le
base = 10    -- do not change......

function fromstr10(s)
    assert(base == 10)
    local a = {}
    for c in s:gmatch"." do
        table.insert(a, 1, string.byte(c, 1) - 48)
    end
    return a
end

function fromint(i)
    local a = {}
    if i == 0 then
        return {0}
    end
    while i ~= 0 do
        table.insert(a, #a + 1, i % base)
        i = i // base
    end
    return a
end

ibase = fromint(base)

function zeros(length)
    local digits = {}
    for i=1,length do
        table.insert(digits, 1, 0)
    end
    return digits
end

function add(a, b)
    local carry = 0
    -- Maximum
    local c = #a >= #b and zeros(#a) or zeros(#b)
    for i=1,#c do
        total = carry
        if i <= #a then
            total = total + a[i]
        end
        if i <= #b then
            total = total + b[i]
        end
        carry = total // base
        c[i] = total % base
    end
    if carry ~= 0 then
        table.insert(c, #c + 1, carry)
    end
    return c
end

function eq(a, b)
    if #a ~= #b then
        return false
    end
    for i=1,#a do
        if a[i] ~= b[i] then
            return false
        end
    end
    return true
end

function lt(a, b)
    if #a ~= #b then
        return #a < #b
    end
    for i=#a,1,-1 do
        if a[i] ~= b[i] then
            return a[i] < b[i]
        end
    end
    return false
end

function le(a, b)
    return lt(a, b) or eq(a, b)
end

function sub(a, b)
    if eq(a, b) then
        return fromint(0)
    end
    if lt(a, b) then
        return fromint(0)
    end
    local c = zeros(#a)
    local carry = 0
    local total
    -- Operate on least significant digit first
    for i=1,#a do
        if i <= #b then
            total = a[i] - b[i] + carry
        else
            total = a[i] + carry
        end
        if total < 0 then
            total = total + base
            carry = -1
        else
            carry = 0
        end
        c[i] = total
    end
    while c[#c] == 0 do
        table.remove(c)
    end
    return c
end

function _mul(a, b)
    local N = #a + #b
    local c = zeros(N)
    for i=1,#a do
        for j=1,#b do
            c[i + j - 1] = c[i + j - 1] + a[i] * b[j]
        end
    end
    carry = 0
    for i=1,N do
        total = c[i] + carry
        c[i] = total % base
        carry = total // base
    end
    while c[#c] == 0 do
        table.remove(c)
    end
    return c
end

function split(t, ix)
    local lo = {}
    local hi = {}
    for i=1,#t do
        local h = i <= ix and lo or hi
        table.insert(h, #h + 1, t[i])
    end
    return lo, hi
end

function mul(a, b)
    -- Karatsuba bigint multiplication algorithm
    -- Limit when either number is 2 digits long or shorter
    if #a <= 2 or #b <= 2 then
        return _mul(a, b)
    end
    -- Max size
    local m2 = (#a >= #b and #a or #b) // 2
    local l1, h1 = split(a, m2)
    local l2, h2 = split(b, m2)
    local k0 = mul(l1, l2)
    local k1 = mul(add(l1, h1), add(l2, h2))
    local k2 = mul(h1, h2)
    -- k0 + (k - k2 - k0) * B^max + k2 * B^2max
    return add(
        _mul(k2, pow(ibase, fromint(m2 * 2))),
        add(
            _mul(sub(k1, add(k2, k0)), pow(ibase, fromint(m2))),
            k0
    ))
end

function halve(a)
    local magic = 0
    local c = zeros(#a)
    for i=#a,1,-1 do
        digit = (a[i] >> 1) + magic
        magic = (a[i] & 1) * (base // 2)
        c[i] = digit
    end
    while c[#c] == 0 do
        table.remove(c)
    end
    return c
end

function divmod(a, b, fmul)
    fmul = fmul or mul
    if null(b) then
        error("cannot divide by zero")
    end
    if lt(a, b) then
        return fromint(0), a
    end
    if eq(a, b) then
        return fromint(1), fromint(0)
    end

    local n = #a
    local i = n
    local mod = fromint(0)
    local div = zeros(n)

    while lt(add(fmul(mod, ibase), fromint(a[i])), b) do
        mod = fmul(mod, ibase)
        mod = add(mod, fromint(a[i]))
        i = i - 1
    end

    local lgcat = 1
    local cat = zeros(n)
    while i > 0 do
        mod = add(fmul(mod, ibase), fromint(a[i]))

        cc = base - 1
        -- ??? mod < b cc
        while lt(mod, fmul(b, fromint(cc))) do
            cc = cc - 1
        end

        mod = sub(mod, fmul(b, fromint(cc)))
        cat[lgcat] = cc
        lgcat = lgcat + 1
        i = i - 1
    end

    for i=1,lgcat do
        div[i] = cat[lgcat - i]
    end
    div, _ = split(div, lgcat)

    return div, mod
end

function div(a, b)
    d, m = divmod(a, b)
    return d
end

function null(a)
    for i=1,#a do
        if a[i] ~= 0 then
            return false
        end
    end
    return true
end

function pow(a, b, fmul)
    fmul = fmul or _mul
    local exponent = b
    local v = a
    local result = fromint(1)
    while not null(exponent) do
        if exponent[1] & 1 == 1 then
            result = fmul(result, v)
        end
        v = fmul(v, v)
        exponent = halve(exponent)
    end
    return result
end

function slowpow(a, b, fmul)
    fmul = fmul or mul
    local result = fromint(1)
    for i=1,int(b) do
        result = fmul(a, result)
    end
    return result
end

function sqrt(a)
    local left = fromint(1)
    local right = a
    local v = fromint(1)
    local mid = fromint(0)
    local prod = fromint(0)
    while le(left, right) do
        mid = add(mid, left)
        mid = add(mid, right)
        mid = halve(mid)
        prod = mul(mid, mid)
        if le(prod, a) then
            v = mid
            mid = add(mid, fromint(1))
            left = mid
        else
            mid = sub(mid, fromint(1))
            right = mid
        end
        mid = fromint(0)
    end
    return v
end

function slow_e_str()
    local digits = 1000
    assert(digits == 1000)
    local result = fromint(0)
    local factor = pow(fromint(10), fromint(digits + 5))
    for i=2,480 do
        result = add(result, div(factor, factorial(i)))
    end
    return "2." .. show10(result)
end

function e_str()
    local result = fromint(0)
    local digits = 400
    local factor = pow(fromint(10), fromint(digits))
    -- Compute factorial as we go, instead of multiple times
    local fac = fromint(1)
    for i=400,1,-1 do
        fac = mul(fac, fromint(i))
        result = add(result, fac)
    end
    -- result = result / f
    result = div(mul(result, factor), fac)
    return "2." .. string.sub(show10(result), 2)
end

function bigish_prime()
    -- the famous 25519 prime
    local prime = sub(pow(fromint(2), fromint(255)), fromint(19))
    return show10(prime)
end

function factorial(n)
    local v = fromint(1)
    for i=1,n do
        v = mul(v, fromint(i))
    end
    return v
end

function show10(t)
    local log10 = #(base .. "") - 1
    local result = {}
    for i=1,#t do
        table.insert(result, 1, t[i])
    end
    return table.concat(result)
end

function show(t)
    local sbase = base .. ""
    local is10 = sbase[1] 
    for c=2,#sbase do
        is10 = is10 and sbase[s] == "0"
    end
    -- TODO! zfill, check if base is log10
    return show10(t)
end

function int(a)
    local result = 0
    local basen = 1
    for i=1,#a do
        result = result + basen * a[i]
        basen = basen * base
    end
    return result
end

function test()
    -- assert(base == 10)
    -- print(show10(fromstr10("123456")))
    print()
    print('sub ' .. (123456 - 231))
    print(show10(sub(fromint(123456), fromint(231))))
    print(show10(sub(fromint(123456), fromint(123455))))
    print(show10(sub(fromint(123456), fromint(123456))))
    print()
    print('div ' .. (123456 // 231))
    print(show10(div(fromint(123456), fromint(231))))
    print(show10(div(fromint(123456), fromint(123456))))
    print()
    for i=1,2 do
        m = ({_mul, mul})[i]
        print(show10(m(fromint(2), fromint(3))))
        print(show10(m(fromint(123456), fromint(231))))
        local t = fromint(100)
        print(show10(m(t, t)))
        print()
    end
    for i=1,2 do
        p = ({pow, slowpow})[i]
        print(show10(p(fromint(2), fromint(100))))
        print(show10(p(fromint(1000), fromint(100))))
        print(show10(p(fromint(2), fromint(1000), _mul)))
        print(show10(p(fromint(2), fromint(1000), mul)))
        print()
    end
    print(show10(halve(fromint(123456))))
    print(show10(halve(fromint(123455))))
    print()
    print(show10(pow(halve(mul(add({1, 2, 3}, {4, 5}), {3, 2, 1})), fromint(100), mul)))
    print(show10(factorial(50)))
    print()
    print(show10(sqrt(fromint(2000000000))))
    print(show10(sqrt(pow(fromint(20), fromint(50)))))
    -- print(e_str())
end

print(test())
-- print(bigish_prime())
-- print(e_str())
