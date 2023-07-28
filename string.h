#include <iostream>
#include <cstring>

class String
{
public:
    String();
    String(size_t n, char c);
    String(const char* s);
    String(const String& s);
    ~String();

    String& operator=(const String& s);
    String& operator+=(const String& s);
    String& operator+=(char c);
    String operator+(const String& s) const;
    String operator+(char c) const;
    bool operator==(const String& s) const;

    void push_back(char c);
    void pop_back();
    void clear();

    size_t find(const String& sub) const;
    size_t rfind(const String& sub) const;

    String substr(size_t start, size_t count) const;

    constexpr char& operator[](size_t i) { return buf[i]; }
    constexpr const char& operator[](size_t i) const { return buf[i]; }
    
    constexpr size_t length() const { return len; }
    constexpr bool empty() const { return !len; }

    constexpr char& front() { return buf[0]; }
    constexpr char& back() { return buf[len - 1]; }
    constexpr const char& front() const { return buf[0]; }
    constexpr const char& back() const { return buf[len - 1]; }
private:
    void resize(size_t new_len, bool copy=true);
private:
    size_t len, cap;
    char* buf;
};

String::String() : len(0), cap(0), buf(nullptr) 
{
}

String::String(size_t n, char c) : len(0), cap(0), buf(nullptr)
{
    if(!n) return;
    resize(n);
    memset(buf, c, n);
}

String::String(const char* s) : len(0), cap(0), buf(nullptr)
{
    if(!*s) return;
    resize(strlen(s));
    memcpy(buf, s, len);
}

String::String(const String& s) : len(0), cap(0), buf(nullptr)
{
    if(!s.len) return;
    resize(s.len);
    memcpy(buf, s.buf, len);
}

String::~String()
{ 
    delete[] buf;
}

String& String::operator=(const String& s)
{
    if(s.buf == buf) return *this;
    resize(s.len, false);
    memcpy(buf, s.buf, len);
    return *this;
}

String String::operator+(const String& s) const
{
    String res = *this;
    return res += s;
}

String String::operator+(char c) const
{
    String res = *this;
    return res += c;
}

String operator+(char c, const String& s)
{
    String res(1, c);
    return res += s;
}

String& String::operator+=(const String& s)
{
    size_t l = len, sl = s.len;
    if(!sl) return *this;
    resize(l + sl);
    memcpy(buf + l, s.buf, sl);
    return *this;
}

String& String::operator+=(char c)
{
    push_back(c);
    return *this;
}

bool String::operator==(const String& s) const
{
    if(len != s.len) return false;
    if(!len) return true;
    return !memcmp(buf, s.buf, len);
}

void String::push_back(char c)
{
    resize(len + 1);
    buf[len - 1] = c;
}

void String::pop_back()
{
    len--;
}

void String::clear()
{
    if(buf) delete[] buf;
    buf = nullptr;
    len = cap = 0;
}

size_t String::find(const String& sub) const
{
    if(!sub.len || sub.len > len) return len;
    for(size_t i = 0; i <= len - sub.len; i++) {
        if(buf[i] != sub[0]) continue;
        for(size_t j = 0; true; j++) {
            if(buf[i + j] != sub[j]) break;
            if(j == sub.len - 1) return i;
        }
    }
    return len;
}

size_t String::rfind(const String& sub) const
{
    if(!sub.len || sub.len > len) return len;
    for(size_t i = len - sub.len; i < len; i--) {
        if(buf[i] != sub[0]) continue;
        for(size_t j = 0; true; j++) {
            if(buf[i + j] != sub[j]) break;
            if(j == sub.len - 1) return i;
        }
    }
    return len;
}

String String::substr(size_t start, size_t count) const
{
    String res;
    if(!count) return res;
    res.resize(count);
    memcpy(res.buf, buf + start, count);
    return res;
}

void String::resize(size_t new_len, bool copy)
{
    if(new_len < cap) {
        len = new_len;
        return;
    }

    char* tmp = buf;
    cap = new_len * 2;
    buf = new char[cap];
    
    if(tmp) {
        if(copy) memcpy(buf, tmp, len);
        delete[] tmp;
    }
    len = new_len;
}

std::istream& operator>>(std::istream& is, String& s)
{
    s.clear();
    char c;
    while(isspace(c = is.get()))
        if(c == EOF) return is;
    do {
        if(c == EOF) return is;
        s.push_back(c);
    } while(!isspace(c = is.get()));
    is.unget();
    return is;
}

std::ostream& operator<<(std::ostream& os, const String& s)
{
    for(size_t i = 0; i < s.length(); i++) os << s[i];
    return os;
}
