#include "biginteger.h"

template<size_t N>
class Residue
{
public:
    Residue() = default;
    Residue(const Residue& r) = default;
    Residue& operator=(const Residue& r) = default;
    explicit Residue(int value) : value(normalize(value)) { }
    explicit operator int() const { return value; }
    explicit operator bool() const { return value; }

    Residue<N>& operator+=(const Residue<N>& r) { value = normalize(value + r.value); return *this; }
    Residue<N>& operator-=(const Residue<N>& r) { value = normalize(value - r.value); return *this; }
    Residue<N>& operator*=(const Residue<N>& r) { value = normalize(value * r.value); return *this; }
    Residue<N>& operator/=(const Residue<N>& r) { return *this *= r.inverse(); }
private:
    Residue<N> inverse() const;
    static int normalize(int n) { return (((n % (int)N) + (int)N) % (int)N); }
private:
    int value;
private:
    template<size_t n, size_t div>
    struct is_prime
    {
        static const bool value = n % div && is_prime<n, div - 1>::value;
    };
    template<size_t n> struct is_prime<n, 1> { static const bool value = false; };
    template<size_t n> struct is_prime<n, 2> { static const bool value = true; };

    template<size_t K>
    friend bool operator==(const Residue<K>& a, const Residue<K>& b);
};

template<size_t M, size_t N, typename Field = Rational>
class Matrix
{
public:
    Matrix();
    Matrix(const Matrix& m) = default;

    template<typename T> Matrix(const std::vector<std::vector<T>>& s);
    template<typename T> Matrix(const std::initializer_list<std::initializer_list<T>>& s);

    Matrix& operator=(const Matrix& m) = default;

    Matrix<M, N, Field>& operator+=(const Matrix<M, N, Field>& m);
    Matrix<M, N, Field>& operator-=(const Matrix<M, N, Field>& m);
    Matrix<M, N, Field>& operator*=(const Field& f);
    Matrix<M, N, Field>& operator*=(const Matrix<M, N, Field>& m);

    Field det() const;
    Field trace() const;
    size_t rank() const;

    Matrix<N, M, Field> transposed() const;
    Matrix<M, N, Field> inverted() const;

    Matrix<M, N, Field>& invert();

    std::vector<Field> getRow(size_t i) const;
    std::vector<Field> getColumn(size_t i) const;

    std::vector<Field>& operator[](size_t i) { return source[i]; }
    const std::vector<Field>& operator[](size_t i) const { return source[i]; }
    
    template<size_t K> Field linear_com(const Matrix<N, K, Field>& m, size_t i, size_t j) const;
private:
    void square_matrix() const;
    void allocate();
    void sub_row(size_t dst, size_t src);
    void sub_row(size_t dst, size_t src, const Field& k);
    void gauss(Matrix<M, N, Field>* unity, int delta);      // +1 from top to bottom, -1 from bottom to top
private:
    std::vector<std::vector<Field>> source;
};

template<size_t M, size_t N, typename Field>
std::ostream& operator<<(std::ostream& os, const Matrix<M, N, Field>& m)
{
    m.getColumn(0);
    return os;
}

template<size_t M, size_t N, size_t R>
std::ostream& operator<<(std::ostream& os, const Matrix<M, N, Residue<R>>& m)
{
    os << M << ", " << N << ", " << R << std::endl;
    os << "{ ";
    for(size_t i = 0; i < M; i++) {
        if(i) os << "  ";
        os << "{ ";
        for(size_t j = 0; j < N; j++) {
            os << (int)m[i][j];
            if(j != N - 1) os << ", ";
        }
        os << " }," << std::endl;
    }
    return os << "};" << std::endl;
}

template<size_t M, size_t N>
std::ostream& operator<<(std::ostream& os, const Matrix<M, N>& m)
{
    os << M << ", " << N << std::endl;
    os << "{ ";
    for(size_t i = 0; i < M; i++) {
        if(i) os << "  ";
        os << "{ ";
        for(size_t j = 0; j < N; j++) {
            os << m[i][j].toString();
            if(j != N - 1) os << ", ";
        }
        os << " }," << std::endl;
    }
    return os << "};" << std::endl;
}

template<size_t N, typename Field = Rational>
using SquareMatrix = Matrix<N, N, Field>;

template<size_t M, size_t N, typename Field>
Matrix<M, N, Field>::Matrix()
{
    allocate();
    for(size_t i = 0; i < M; i++)
        for(size_t j = 0; j < N; j++)
            (*this)[i][j] = Field(i == j);
}

template<size_t M, size_t N, typename Field>
template<typename T>
Matrix<M, N, Field>::Matrix(const std::vector<std::vector<T>>& s)
{
    allocate();
    for(size_t i = 0; i < M; i++)
        for(size_t j = 0; j < N; j++)
            (*this)[i][j] = Field(s[i][j]);
}

template<size_t M, size_t N, typename Field>
template<typename T>
Matrix<M, N, Field>::Matrix(const std::initializer_list<std::initializer_list<T>>& s)
{
    allocate();
    size_t i = 0;
    for(const auto& x : s) {
        size_t j = 0;
        for(const auto& y : x)
            (*this)[i][j++] = Field(y);
        i++;
    }
}

template<size_t M, size_t N, typename Field>
Matrix<M, N, Field>& Matrix<M, N, Field>::operator+=(const Matrix<M, N, Field>& m)
{
    for(size_t i = 0; i < M; i++)
        for(size_t j = 0; j < N; j++)
            (*this)[i][j] += m[i][j];
    return *this;
}

template<size_t M, size_t N, typename Field>
Matrix<M, N, Field>& Matrix<M, N, Field>::operator-=(const Matrix<M, N, Field>& m)
{
    for(size_t i = 0; i < M; i++)
        for(size_t j = 0; j < N; j++)
            (*this)[i][j] -= m[i][j];
    return *this;
}

template<size_t M, size_t N, typename Field>
Matrix<M, N, Field>& Matrix<M, N, Field>::operator*=(const Field& f)
{
    for(auto& i : source)
        for(auto& j : i) j *= f;
    return *this;
}

template<size_t M, size_t N, typename Field>
Matrix<M, N, Field>& Matrix<M, N, Field>::operator*=(const Matrix<M, N, Field>& m)
{
    square_matrix();

    for(size_t i = 0; i < N; i++) {
        Matrix<1, N, Field> tmp = std::vector<std::vector<Field>>({ (*this)[i] });
        for(size_t j = 0; j < N; j++)
            (*this)[i][j] = tmp.linear_com(m, 0, j);
    }
    return *this;
}

template<size_t M, size_t N, typename Field>
Field Matrix<M, N, Field>::det() const
{
    square_matrix();
    Field res = Field(1);
    Matrix m = *this;
    
    m.gauss(nullptr, 1);
    m.gauss(nullptr, -1);

    for(size_t i = 0; i < N; i++)
        res *= m[i][i];
    return res;
}

template<size_t M, size_t N, typename Field>
Field Matrix<M, N, Field>::trace() const
{
    square_matrix();
    Field res = Field(0);
    for(size_t i = 0; i < N; i++)
        res += (*this)[i][i];
    return res;
}

template<size_t M, size_t N, typename Field>
size_t Matrix<M, N, Field>::rank() const
{
    size_t res = 0;
    Matrix<M, N, Field> tmp = *this;
    tmp.gauss(nullptr, 1);
    for(const auto& i : tmp.source)
        for(const auto& j : i)
            if(j) {
                res++;
                break;
            }
    return res;
}

template<size_t M, size_t N, typename Field>
Matrix<N, M, Field> Matrix<M, N, Field>::transposed() const
{
    Matrix<N, M, Field> m;
    for(size_t i = 0; i < M; i++)
        for(size_t j = 0; j < N; j++)
            m[j][i] = (*this)[i][j];
    return m;
}

template<size_t M, size_t N, typename Field>
Matrix<M, N, Field> Matrix<M, N, Field>::inverted() const
{
    square_matrix();
    Matrix<M, N, Field> res = *this;
    res.invert();
    return res;
}

template<size_t M, size_t N, typename Field>
Matrix<M, N, Field>& Matrix<M, N, Field>::invert()
{
    square_matrix();
    Matrix<M, N, Field> tmp;
    gauss(&tmp, 1);
    gauss(&tmp, -1);

    for(size_t i = 0; i < N; i++)
        for(auto& j : tmp[i])
            j /= (*this)[i][i];

    return *this = tmp;
}

template<size_t M, size_t N, typename Field>
std::vector<Field> Matrix<M, N, Field>::getRow(size_t i) const
{
    return (*this)[i];
}

template<size_t M, size_t N, typename Field>
std::vector<Field> Matrix<M, N, Field>::getColumn(size_t i) const
{
    std::vector<Field> res(M);
    for(size_t j = 0; j < M; j++) res[j] = (*this)[i][j];
    return res;
}

template<size_t M, size_t N, typename Field>
template<size_t K>
Field Matrix<M, N, Field>::linear_com(const Matrix<N, K, Field>& m, size_t i, size_t j) const
{
    Field res = Field(0);
    for(size_t k = 0; k < N; k++)
        res += (*this)[i][k] * m[k][j];
    return res;
}


template<size_t M, size_t N, typename Field>
void Matrix<M, N, Field>::square_matrix() const
{
    static_assert(M == N);
}

template<size_t M, size_t N, typename Field>
void Matrix<M, N, Field>::allocate()
{
    source.resize(M);
    for(auto& i : source) i.resize(N);
}

template<size_t M, size_t N, typename Field>
void Matrix<M, N, Field>::sub_row(size_t dst, size_t src)
{
    for(size_t j = 0; j < N; j++)
        (*this)[dst][j] -= (*this)[src][j];
}

template<size_t M, size_t N, typename Field>
void Matrix<M, N, Field>::sub_row(size_t dst, size_t src, const Field& k)
{
    for(size_t j = 0; j < N; j++)
        (*this)[dst][j] -= (*this)[src][j] * k;
}

template<size_t M, size_t N, typename Field>
void Matrix<M, N, Field>::gauss(Matrix<M, N, Field>* unity, int delta)
{
    for(size_t i = delta > 0 ? 0 : N - 1; i < N; i += delta) {
        if(i >= M) {
            if(delta > 0) break;
            i = M - 1;
        }

        for(size_t t = i; t < M; t += delta) {
            if((*this)[t][i]) {
                if(t != i) {
                    sub_row(i, t);
                    if(unity) (*unity).sub_row(i, t);
                }
                break;
            }
        }
        if(!(*this)[i][i]) continue;
        
        for(size_t j = i + delta; j < M; j += delta) {
            Field f = (*this)[j][i] / (*this)[i][i];
            sub_row(j, i, f);
            if(unity) (*unity).sub_row(j, i, f);
        }
    }
}


template<size_t M, size_t N, typename Field>
Matrix<M, N, Field> operator+(const Matrix<M, N, Field>& a, const Matrix<M, N, Field>& b)
{
    Matrix<M, N, Field> res = a;
    res += b;
    return res;
}

template<size_t M, size_t N, typename Field>
Matrix<M, N, Field> operator-(const Matrix<M, N, Field>& a, const Matrix<M, N, Field>& b)
{
    Matrix<M, N, Field> res = a;
    res -= b;
    return res;
}

template<size_t M, size_t N, typename Field>
Matrix<M, N, Field> operator*(const Matrix<M, N, Field>& m, const Field& f)
{
    Matrix<M, N, Field> res = m;
    res *= f;
    return res;
}

template<size_t M, size_t N, typename Field>
Matrix<M, N, Field> operator*(const Field& f, const Matrix<M, N, Field>& m)
{
    Matrix<M, N, Field> res = m;
    res *= f;
    return res;
}

template<size_t M, size_t N, size_t K, typename Field>
Matrix<M, K, Field> operator*(const Matrix<M, N, Field>& a, const Matrix<N, K, Field>& b)
{
    Matrix<M, K, Field> res;
    
    for(size_t i = 0; i < M; i++)
        for(size_t j = 0; j < K; j++)
            res[i][j] = a.linear_com(b, i, j);
    return res;
}



template<size_t M, size_t N, typename Field>
bool operator==(const Matrix<M, N, Field>& a, const Matrix<M, N, Field>& b)
{
    for(size_t i = 0; i < M; i++)
        for(size_t j = 0; j < N; j++)
            if(a[i][j] != b[i][j]) {
                return false;
            }
    return true;
}

template<size_t M, size_t N, typename Field>
bool operator!=(const Matrix<M, N, Field>& a, const Matrix<M, N, Field>& b)
{
    return !(a == b);
}


template<size_t N>
Residue<N> operator+(const Residue<N>& a, const Residue<N>& b)
{
    Residue<N> res = a;
    res += b;
    return res;
}

template<size_t N>
Residue<N> operator-(const Residue<N>& a, const Residue<N>& b)
{
    Residue<N> res = a;
    res -= b;
    return res;
}

template<size_t N>
Residue<N> operator*(const Residue<N>& a, const Residue<N>& b) 
{
    Residue<N> res = a;
    res *= b;
    return res;
}

template<size_t N>
Residue<N> operator/(const Residue<N>& a, const Residue<N>& b)
{
    Residue<N> res = a;
    res /= b;
    return res;
}

template<size_t N>
bool operator==(const Residue<N>& a, const Residue<N>& b)
{
    return a.value == b.value;
}

template<size_t N>
bool operator!=(const Residue<N>& a, const Residue<N>& b)
{
    return !(a == b);
}

template<size_t N>
Residue<N> Residue<N>::inverse() const
{
    static_assert(is_prime<N, (size_t)std::sqrt((double)N) + 1>::value);
    int i;
    for(i = N - 1; i > 0; i--)
        if((i * (int)*this) % N == 1) break;
    return Residue<N>(i);
}
