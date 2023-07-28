#include <assert.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>

class BigInteger
{
public:
    static const size_t BASE_LOG = 9;
    static const long long BASE = 1000000000;
public:
    BigInteger(long long n);
    BigInteger() = default;
    BigInteger(const BigInteger& b) = default;
    BigInteger(BigInteger&& b) = default;

    BigInteger& operator=(long long n);
    BigInteger& operator=(const BigInteger& b) = default;
    
    BigInteger& operator+=(const BigInteger& b);
    BigInteger& operator-=(const BigInteger& b);
    BigInteger& operator*=(const BigInteger& b);
    BigInteger& operator/=(const BigInteger& b);
    BigInteger& operator%=(const BigInteger& b);
    
    BigInteger& operator++();
    BigInteger& operator--();

    BigInteger operator++(int);
    BigInteger operator--(int);

    BigInteger operator-() const;

    explicit operator bool() const;

    std::string toString() const;

    void change_sign();

    void half();                                                    // *this /= 2
    bool even();
private:
    void inc(size_t i);                                             // add BASE^i with the same sign
    void dec(size_t i);                                             // add BASE^i with the opposite sign, abs(*this) >= BASE^i
    void add(size_t i, const BigInteger& a, const BigInteger& b);   // |*this| = |a| + |b|*BASE^i
    void sub(size_t i, const BigInteger& a, const BigInteger& b);   // |*this| = |a| - |b|*BASE^i, |a| >= |b|*BASE^i
    void divmod(BigInteger* div, const BigInteger& b);
    void normalize();
    size_t count_digits() const;
private:
    static size_t count_digits(long long n, long long base = BASE);
    static bool unsigned_equal(const BigInteger& a, const BigInteger& b);
    static bool unsigned_greater(size_t i, const BigInteger& a, const BigInteger& b);   // |a*BASE^i|>|b|
private:
    bool sign;
    std::vector<long long> digits;
    
    friend std::istream& operator>>(std::istream& os, BigInteger& b);
    friend bool operator==(const BigInteger& a, const BigInteger& b);
    friend bool operator>(const BigInteger& a, const BigInteger& b);
};

class Rational
{
public:
    Rational();
    Rational(long long n);
    Rational(const BigInteger& a);
    Rational(const BigInteger& a, const BigInteger& b);
    Rational(const Rational& r) = default;

    Rational& operator=(const Rational& r) = default;

    Rational& operator+=(const Rational& r);
    Rational& operator-=(const Rational& r);
    Rational& operator*=(const Rational& r);
    Rational& operator/=(const Rational& r);
    
    Rational operator-() const;


    std::string toString() const;
    std::string asDecimal(size_t precision = 0) const;
    
    explicit operator bool() const;
    explicit operator double() const;
private:
    static BigInteger* common_factor(BigInteger* a, BigInteger* b);
private:
    void normalize();
private:
    BigInteger a, b;

    friend bool operator==(const Rational& a, const Rational& b);
    friend bool operator>(const Rational& a, const Rational& b);
    friend std::istream& operator>>(std::istream& is, Rational& r);
};

std::istream& operator>>(std::istream& is, Rational& r)
{
    char c;
    is >> r.a;
    //while(isspace(c = is.get()));
    if((c = is.get()) == '/') is >> r.b;
    else is.unget();
    r.normalize();
    return is;
}

bool operator==(const BigInteger& a, const BigInteger& b);
bool operator!=(const BigInteger& a, const BigInteger& b);
bool operator>(const BigInteger& a, const BigInteger& b);
bool operator<(const BigInteger& a, const BigInteger& b);
bool operator>=(const BigInteger& a, const BigInteger& b);
bool operator<=(const BigInteger& a, const BigInteger& b);

BigInteger operator+(const BigInteger& a, const BigInteger& b);
BigInteger operator-(const BigInteger& a, const BigInteger& b);
BigInteger operator*(const BigInteger& a, const BigInteger& b);
BigInteger operator/(const BigInteger& a, const BigInteger& b);
BigInteger operator%(const BigInteger& a, const BigInteger& b);

std::ostream& operator<<(std::ostream& os, const BigInteger& b);
std::istream& operator>>(std::istream& is, BigInteger& b);

Rational operator+(const Rational& a, const Rational& b); 
Rational operator-(const Rational& a, const Rational& b);
Rational operator*(const Rational& a, const Rational& b);
Rational operator/(const Rational& a, const Rational& b);

bool operator==(const Rational& a, const Rational& b);
bool operator!=(const Rational& a, const Rational& b);
bool operator>(const Rational& a, const Rational& b);
bool operator<(const Rational& a, const Rational& b);
bool operator>=(const Rational& a, const Rational& b);
bool operator<=(const Rational& a, const Rational& b);

BigInteger::BigInteger(long long n) : sign(n < 0), digits(count_digits(n))
{
    size_t i = 0;
    n = std::abs(n);

    while(n) {
        digits[i++] = n % BASE;
        n /= BASE;
    }
}

BigInteger& BigInteger::operator=(long long n)
{
    size_t i = 0;
    sign = n < 0;
    digits.resize(count_digits(n));
    n = std::abs(n);

    while(n) {
        digits[i++] = n % BASE;
        n /= BASE;
    }
    return *this;
}

BigInteger& BigInteger::operator+=(const BigInteger& b)
{
    bool gr = unsigned_greater(0, *this, b);

    if(sign == b.sign) add(0, gr ? *this : b, gr ? b : *this);
    else {
        sub(0, gr ? *this : b, gr ? b : *this);
        if(!gr) change_sign();
    }

    return *this;
}

BigInteger& BigInteger::operator-=(const BigInteger& b)
{
    bool gr = unsigned_greater(0, *this, b);

    if(sign != b.sign) add(0, gr ? *this : b, gr ? b : *this);
    else {
        sub(0, gr ? *this : b, gr ? b : *this);
        if(!gr) change_sign();
    }

    return *this;
}

BigInteger& BigInteger::operator*=(const BigInteger& b)
{
    if(!*this) return *this;
    if(!b) return *this = 0;

    size_t a_digits = count_digits();
    size_t b_digits = b.count_digits();
    size_t high_digit = a_digits + b_digits;

    sign ^= b.sign;
    digits.resize(a_digits + b_digits);

    for(size_t i = a_digits - 1; i < a_digits; i--)
        for(size_t j = b_digits - 1; j < b_digits; j--) {
            long long res = digits[i] * b.digits[j];

            if(i + j >= high_digit) res += digits[i + j];
            else high_digit = i + j;

            digits[i + j] = res % BASE;
            digits[i + j + 1] += res / BASE;

            if(digits[i + j + 1] >= BASE) {
                digits[i + j + 1] %= BASE;
                inc(i + j + 2);
            }
        }

    normalize();

    return *this;
}

BigInteger& BigInteger::operator/=(const BigInteger& b)
{
    BigInteger res = 0;
    divmod(&res, b);
    *this = res;
    return *this;
}

BigInteger& BigInteger::operator%=(const BigInteger& b)
{
    divmod(nullptr, b);
    return *this;
}

BigInteger& BigInteger::operator++()
{
    if(!sign) inc(0);
    else dec(0);

    return *this;
}

BigInteger& BigInteger::operator--()
{
    change_sign();
    ++(*this);
    change_sign();
    return *this;
}

BigInteger BigInteger::operator++(int)
{
    BigInteger b = *this;
    ++(*this);
    return b;
}

BigInteger BigInteger::operator--(int)
{
    BigInteger b = *this;
    --(*this);
    return b;
}

BigInteger BigInteger::operator-() const
{
    BigInteger b = *this;
    b.change_sign();
    return b;
}

BigInteger::operator bool() const
{
    return count_digits();
}

std::string BigInteger::toString() const
{
    std::ostringstream os;

    if(sign) os << '-';
    if(!*this) os << '0';
    for(size_t i = count_digits() - 1; i < count_digits(); i--) {
        size_t n = (i != count_digits() - 1) ? BASE_LOG - count_digits(digits[i], 10) : 0;
        if(n && !digits[i]) n--;
        while(n--) os << '0';
        os << digits[i];
    }
    return os.str();
}


void BigInteger::change_sign()
{
    if(*this) sign = !sign;
    else sign = false;
}

void BigInteger::inc(size_t i)
{
    if(count_digits() <= i) digits.resize(i + 1);
    for(; i < count_digits(); i++) {
        digits[i] = (digits[i] + 1) % BASE;
        if(digits[i]) return;
    }
    digits.push_back(1);
}

void BigInteger::dec(size_t i)
{
    for(; i < count_digits(); i++) {
        digits[i] = (digits[i] + BASE - 1) % BASE;
        if(digits[i] != BASE - 1) break;
    }
    normalize();
}

void BigInteger::add(size_t i, const BigInteger& a, const BigInteger& b)
{
    size_t j = 0;
    long long add = 0;
    size_t a_digits = a.count_digits();
    size_t b_digits = b.count_digits();

    digits.resize(std::max(a.count_digits(), b.count_digits() + i));

    for(; j < i; j++) digits[j] = a.digits[j];
    for(; j < i + b_digits; j++) {
        digits[j] = a.digits[j] + b.digits[j - i] + add;
        add = digits[j] >= BASE;
        digits[j] = digits[j] % BASE;
    }
    for(; j < a_digits; j++) digits[j] = a.digits[j];
    if(add) inc(b_digits + i);
    normalize();
}

void BigInteger::sub(size_t i, const BigInteger& a, const BigInteger& b)
{
    size_t j = 0;
    long long add = 0;
    size_t a_digits = a.count_digits();
    size_t b_digits = b.count_digits();

    digits.resize(std::max(a.count_digits(), b.count_digits() + i));

    for(; j < i; j++) digits[j] = a.digits[j];
    for(; j < i + b_digits; j++) {
        digits[j] = a.digits[j] - b.digits[j - i] - add;
        add = digits[j] < 0;
        digits[j] = (digits[j] + BASE) % BASE;
    }
    for(; j < a_digits; j++) digits[j] = a.digits[j];
    if(add) dec(b_digits + i);
    normalize();
}

void BigInteger::half()
{
    long long add = 0;
    for(size_t i = count_digits() - 1; i < count_digits(); i--) {
        digits[i] += add * BASE;
        add = digits[i] % 2;
        digits[i] /= 2;
    }
    normalize();
}

bool BigInteger::even()
{
    return count_digits() && !(digits[0] & 1);
}

void BigInteger::divmod(BigInteger* div, const BigInteger& b)
{
    assert((bool)b);
    if(div) (*div).sign = b.sign ^ sign;

    while(!unsigned_greater(0, b, *this)) {
        if(!*this) break;

        size_t n = count_digits() - b.count_digits();
        if(unsigned_greater(n, b, *this)) n--;
        
        BigInteger mod = b, ddiv = 1;
        while(!unsigned_greater(n, mod, *this)) {
            mod.add(0, mod, mod);
            ddiv.add(0, ddiv, ddiv);
        }
        mod.half();
        ddiv.half();

        sub(n, *this, mod);
        if(div) (*div).add(n, *div, ddiv);
    }
    if(div) (*div).normalize();
}

void BigInteger::normalize()
{
    while(count_digits() && !digits[count_digits() - 1]) digits.pop_back();
    if(!*this) change_sign();
}

size_t BigInteger::count_digits() const
{
    return digits.size();
}


size_t BigInteger::count_digits(long long n, long long base)
{
    size_t nr = 0;
    long long nn = std::abs(n);
    while(nn) {
        nr++;
        nn /= base;
    }
    return nr;
}

bool BigInteger::unsigned_equal(const BigInteger& a, const BigInteger& b)
{
    return a.digits == b.digits;
}

bool BigInteger::unsigned_greater(size_t i, const BigInteger& a, const BigInteger& b)
{
    if(!a) return false;
    if(a.count_digits() + i > b.count_digits()) return true;
    if(a.count_digits() + i < b.count_digits()) return false;

    for(size_t j = a.count_digits() - 1; j < a.count_digits(); j--) {
        if(a.digits[j] > b.digits[j + i]) return true;
        if(a.digits[j] < b.digits[j + i]) return false;
    }
    return false;
}


std::ostream& operator<<(std::ostream& os, const BigInteger& b) 
{
    return os << b.toString();
}

std::istream& operator>>(std::istream& is, BigInteger& b)
{
    char c;

    while(isspace(c = is.get()));

    if(c == '-') b.sign = true;
    else {
        b.sign = false;
        is.unget();
    }

    while(isspace(c = is.get()));
    while(c == '0') c = is.get();
    is.unget();

    std::string s;
    while((isdigit(c = is.get()))) s.push_back(c);
    is.unget();

    long long n = 0;
    size_t big_digit = s.size() / BigInteger::BASE_LOG, small_digit = s.size() % BigInteger::BASE_LOG;
    if(!small_digit) {
        small_digit = BigInteger::BASE_LOG;
        big_digit--;
    }
    b.digits.resize(big_digit + 1);

    for(auto c : s)
    {
        n = n * 10 + c - '0';
        if(!--small_digit) {
            b.digits[big_digit--] = n;
            small_digit = BigInteger::BASE_LOG;
            n = 0;
        }
    }
    
    return is;
}


bool operator==(const BigInteger& a, const BigInteger& b)
{
    if(a.sign != b.sign) return false;
    return BigInteger::unsigned_equal(a, b);
}

bool operator!=(const BigInteger& a, const BigInteger& b)
{
    return !(a == b);
}

bool operator>(const BigInteger& a, const BigInteger& b)
{
    if(!a.sign && b.sign) return true;
    if(a.sign && !b.sign) return false;
    
    return BigInteger::unsigned_greater(0, a.sign ? b : a, a.sign ? a : b);
}

bool operator<(const BigInteger& a, const BigInteger& b)
{
    return b > a;
}

bool operator>=(const BigInteger& a, const BigInteger& b)
{
    return !(a < b);
}

bool operator<=(const BigInteger& a, const BigInteger& b)
{
    return !(a > b);
}

BigInteger operator+(const BigInteger& a, const BigInteger& b)
{
    BigInteger res = a;
    res += b;
    return res;
}

BigInteger operator-(const BigInteger& a, const BigInteger& b)
{
    BigInteger res = a;
    res -= b;
    return res;
}

BigInteger operator*(const BigInteger& a, const BigInteger& b)
{
    BigInteger res = a;
    res *= b;
    return res;
}

BigInteger operator/(const BigInteger& a, const BigInteger& b)
{
    BigInteger res = a;
    res /= b;
    return res;
}

BigInteger operator%(const BigInteger& a, const BigInteger& b)
{
    BigInteger res = a;
    res %= b;
    return res;
}




Rational::Rational() : a(0), b(1) { }
Rational::Rational(long long n) : a(n), b(1) { }
Rational::Rational(const BigInteger& a) : a(a), b(1) { }
Rational::Rational(const BigInteger& a, const BigInteger& b) : a(a), b(b) { }

Rational& Rational::operator+=(const Rational& r)
{
    //BigInteger bb = b * r.b;
    a = a * r.b + b * r.a;
    b *= r.b;
    normalize();
    return *this;
}

Rational& Rational::operator-=(const Rational& r)
{
    //BigInteger bb = b * r.b;
    a = a * r.b - b * r.a;
    b = b * r.b;
    normalize();
    return *this;
}

Rational& Rational::operator*=(const Rational& r)
{
    a *= r.a;
    b *= r.b;
    normalize();
    return *this;
}

Rational& Rational::operator/=(const Rational& r)
{
    //BigInteger aa = r.a;
    a *= r.b;
    b *= r.a;
    normalize();
    return *this;
}


Rational Rational::operator-() const
{
    Rational r = *this;
    r.a.change_sign();
    return r;
}

std::string Rational::toString() const
{
    std::string s = a.toString();
    if(b != 1) (s += '/') += b.toString();
    return s;
}

std::string Rational::asDecimal(size_t precision) const
{
    BigInteger x = a % b, y = a / b;
    std::string s = x >= 0 ? "" : "-";
    if(x < 0) x.change_sign();
    s += y.toString();
    if(!precision) return s;

    s.reserve(s.size() + 1 + precision);
    s.push_back('.');

    for(size_t i = 0; i < precision; i++) x *= 10;
    std::string t = (x /= b).toString();

    if(t.size() >= precision) {
        for(size_t i = 0; i < t.size(); i++)
            s.push_back(t[i]);
    }
    else {
        for(size_t i = 0; i < precision - t.size(); i++)
            s.push_back('0');
        s += t;
    }

    return s;
}

Rational::operator bool() const
{
    return (bool)a;
}

Rational::operator double() const
{
    std::string s = asDecimal(24);
    return std::atof(s.c_str());
}

BigInteger* Rational::common_factor(BigInteger* a, BigInteger* b)
{
    while(*b) {
        *a %= *b;
        std::swap(a, b);
    }
    return a;
}

void Rational::normalize()
{
    if(b < 0) {
        b.change_sign();
        a.change_sign();
    }
    BigInteger x = a, y = b;
    if(x < 0) x.change_sign();

    bool gr = x > y;
    
    BigInteger* c = common_factor(gr ? &x : &y, gr ? &y : &x);
    a /= *c;
    b /= *c;
}


bool operator==(const Rational& a, const Rational& b)
{
    return a.a == b.a && a.b == b.b;
}

bool operator!=(const Rational& a, const Rational& b)
{
    return !(a == b);
}

bool operator>(const Rational& a, const Rational& b)
{
    return a.a * b.b > a.b * b.a;
}

bool operator<(const Rational& a, const Rational& b)
{
    return b > a;
}

bool operator>=(const Rational& a, const Rational& b)
{
    return !(a < b);
}

bool operator<=(const Rational& a, const Rational& b)
{
    return !(a > b);
}


Rational operator+(const Rational& a, const Rational& b)
{
    Rational res = a;
    res += b;
    return res;
}

Rational operator-(const Rational& a, const Rational& b)
{
    Rational res = a;
    res -= b;
    return res;
}

Rational operator*(const Rational& a, const Rational& b)
{
    Rational res = a;
    res *= b;
    return res;
}

Rational operator/(const Rational& a, const Rational& b)
{
    Rational res = a;
    res /= b;
    return res;
}
