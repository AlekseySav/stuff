#include <vector>
#include <tuple>
#include <iostream>
#include <cmath>
#include <assert.h>
#include <stdint.h>

#define TRACE_POINT(p) std::cerr << "P(" << p.x << ", " << p.y << "),\n"

#define TRACE_POLYGON(p) do { \
    std::cerr.precision(10); \
    for(auto v : (p).getVertices()) \
        TRACE_POINT(v); \
    std::cerr << '\n'; } \
    while(0)

#define TRACE_ELLIPSE(e) do { \
    TRACE_POINT((e).f1); \
    TRACE_POINT((e).f2); \
    std::cerr << (e).distances << std::endl; } \
    while(0);

#define NOT_IMPLEMENTED     std::cerr << "noimpl:" << __PRETTY_FUNCTION__ << std::endl

const double ACCURACY = 0.0001;

struct Point
{
    double x, y;

    Point() = default;
    Point(double x, double y);

    void rotate(const Point& center, double angle);
    void reflect(const Point& center);
    void reflect(const class Line& axis);
    void scale(const Point& center, double coefficient);

    static double distance(const Point& a, const Point& b);
    static double signedArea(const Point& a, const Point& b);
};

class Line
{
public:
    Line(double k, double b);
    Line(const Point& a, double k);
    Line(const Point& a, const Point& b);
    Line(const Line& normal, const Point& center, double offset);
public:
    double k, b;
    friend bool operator==(const Line& a, const Line& b);
    friend Point operator&(const Line& a, const Line& b);
    friend void Point::reflect(const Line& axis);
};

class Shape
{
public:
    virtual ~Shape() { }

    virtual double perimeter() const = 0;
    virtual double area() const = 0;
    virtual bool operator==(const Shape& another) const = 0;
    virtual bool operator!=(const Shape& another) const = 0;
    virtual bool isCongruentTo(const Shape& another) const = 0;
    virtual bool isSimilarTo(const Shape& another) const = 0;
    virtual bool containsPoint(const Point& point) const = 0;

    virtual void rotate(const Point& center, double angle) = 0;
    virtual void reflect(const Point& center) = 0;
    virtual void reflect(const Line& axis) = 0;
    virtual void scale(const Point& center, double coefficient) = 0;
};

class Polygon : public Shape
{
public:
    template<typename... T>
    Polygon(T... v);

    size_t verticesCount() const;
    const std::vector<Point>& getVertices() const;
    bool isConvex() const;

    virtual double perimeter() const override;
    virtual double area() const override;
    virtual bool operator==(const Shape& another) const override;
    virtual bool operator!=(const Shape& another) const override;
    virtual bool isCongruentTo(const Shape& another) const override;
    virtual bool isSimilarTo(const Shape& another) const override;
    virtual bool containsPoint(const Point& point) const override;

    virtual void rotate(const Point& center, double angle) override;
    virtual void reflect(const Point& center) override;
    virtual void reflect(const Line& axis) override;
    virtual void scale(const Point& center, double coefficient) override;
    
    Point polygonCenter() const;
    Point nearestVertex() const;
    void translate(const Point& p);
    bool isCongruentTo(const Polygon& p, const Point& c, double angle) const;
protected:
    std::vector<Point> vertices;
};

class Ellipse : public Shape
{
public:
    Ellipse(const Point& f1, const Point& f2, double distances);

    std::pair<Point, Point> focuses() const;
    std::pair<Line, Line> directrices() const;
    double eccentricity() const;
    Point center() const;

    virtual double perimeter() const override;
    virtual double area() const override;
    virtual bool operator==(const Shape& another) const override;
    virtual bool operator!=(const Shape& another) const override;
    virtual bool isCongruentTo(const Shape& another) const override;
    virtual bool isSimilarTo(const Shape& another) const override;
    virtual bool containsPoint(const Point& point) const override;

    virtual void rotate(const Point& center, double angle) override;
    virtual void reflect(const Point& center) override;
    virtual void reflect(const Line& axis) override;
    virtual void scale(const Point& center, double coefficient) override;
private:
    Point f1, f2;
protected:
    double distances;
};

class Circle : public Ellipse
{
public:
    Circle(const Point& center, double radius);
    double radius() const;
};

class Rectangle : public Polygon
{
public:
    Rectangle(const Point& a, const Point& b, double ratio);

    Point center() const;
    std::pair<Line, Line> diagonals() const;
};

class Square : public Rectangle
{
public:
    Square(const Point& a, const Point& b);

    Circle circumscribedCircle() const;
    Circle inscribedCircle() const;
};

class Triangle : public Polygon
{
public:
    Triangle(const Point& a, const Point& b, const Point& c);

    Circle circumscribedCircle() const;
    Circle inscribedCircle() const;
    Point centroid() const;
    Point orthocenter() const;
    Line EulerLine() const;
    Circle ninePointsCircle() const;
};

Point::Point(double x, double y) : x(x), y(y)
{ }

void Point::rotate(const Point& center, double angle)
{
    double tmp_x;
    angle = angle * M_PI / 180;

    tmp_x = center.x + (x - center.x) * cos(angle) - (y - center.y) * sin(angle);
    y = center.y + (x - center.x) * sin(angle) + (y - center.y) * cos(angle);
    x = tmp_x;
}

void Point::reflect(const Point& center)
{
    x = center.x * 2 - x;
    y = center.y * 2 - y;
}

void Point::reflect(const Line& axis)
{
    double tmp_x;
    tmp_x = (2 * y - 2 * axis.b - axis.k * x + 1 / axis.k * x) / (1 / axis.k + axis.k);
    y = (2 * x + 2 / axis.k * axis.b + axis.k * y - 1 / axis.k * y) / (1 / axis.k + axis.k);
    x = tmp_x;
}

void Point::scale(const Point& center, double coefficient)
{
    x = center.x + (x - center.x) * coefficient;
    y = center.y + (y - center.y) * coefficient;
}


double Point::distance(const Point& a, const Point& b)
{
    return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

double Point::signedArea(const Point& a, const Point& b)
{
    return (a.x * b.y - a.y * b.x) / 2;
}

bool operator==(const Point& a, const Point& b)
{
    return std::abs(a.x - b.x) < ACCURACY && std::abs(a.y - b.y) < ACCURACY;
}

bool operator!=(const Point& a, const Point& b)
{
    return !(a == b);
}

Point operator+(const Point& a, const Point& b)
{
    return { a.x + b.x, a.y + b.y };
}

Point operator-(const Point& a, const Point& b)
{
    return { a.x - b.x, a.y - b.y };
}

Point operator*(const Point& p, double x)
{
    return Point { p.x * x, p.y * x };
}

Point operator/(const Point& p, double x)
{
    return Point { p.x / x, p.y / x };
}

Line::Line(double k, double b) : k(k), b(b)
{ }

Line::Line(const Point& a, double k) : Line(k, a.y - a.x * k)
{ }

Line::Line(const Point& a, const Point& b) : Line(a, (a.y - b.y) / (a.x - b.x))
{ }

Line::Line(const Line& normal, const Point& center, double offset) : Line(center, -1 / normal.k)
{
    b -= std::sqrt(offset * k * k / (1 + k * k));
    if(!normal.k) {
        k = INFINITY;
        b = center.x + offset;
    }
}

bool operator==(const Line& a, const Line& b)
{
    return std::abs(a.k - b.k) < ACCURACY && std::abs(a.b - b.b) < ACCURACY;
}

bool operator!=(const Line& a, const Line& b)
{
    return !(a == b);
}

Point operator&(const Line& a, const Line& b)
{
    if(a.k == INFINITY)
        return Point(a.b, b.k * a.b + b.b);
    if(b.k == INFINITY)
        return Point(b.b, a.k * b.b + a.b);
    double x = (b.b - a.b) / (a.k - b.k);
    return Point(x, a.k * x + a.b);
}

template<typename... T>
Polygon::Polygon(T... v) : vertices({ v... })
{ }

size_t Polygon::verticesCount() const
{
    return vertices.size();
}

const std::vector<Point>& Polygon::getVertices() const
{
    return vertices;
}

bool Polygon::isConvex() const
{
    int prev_sign = 0;

    for(size_t i = 0; i < verticesCount(); i++) {
        size_t j = (i + 1) % verticesCount(), k = (j + 1) % verticesCount();
        const Point& a = vertices[i];
        const Point& b = vertices[j];
        const Point& c = vertices[k];
        int sign = (Point::signedArea(b - a, c - b) > 0) + 1;
        if(prev_sign && sign != prev_sign) return false;
        prev_sign = sign;
    }
    return true;
}


double Polygon::perimeter() const
{
    double res = 0;
    for(size_t i = 0; i < verticesCount(); i++)
        res += Point::distance(vertices[i], vertices[(i + 1) % verticesCount()]);
    return res;
}

double Polygon::area() const
{
    double res = 0;
    for(size_t i = 0; i < verticesCount(); i++)
        res += Point::signedArea(vertices[i], vertices[(i + 1) % verticesCount()]);
    return std::abs(res);
}

bool Polygon::operator==(const Shape& another) const
{
    if(!dynamic_cast<const Polygon*>(&another)) return false;
    const Polygon& p = *(const Polygon*)&another;

    if(verticesCount() != p.verticesCount()) return false;

    for(size_t i = 0; i < p.verticesCount(); i++)
        if(p.vertices[i] == vertices[0]) {
            int step = (p.vertices[(i + 1) % p.verticesCount()] == vertices[1]) ? 1 : -1;
            for(size_t j = 0; j < verticesCount(); j++) {
                if(vertices[j] != p.vertices[i])
                    return false;
                i = (i + step + verticesCount()) % verticesCount();
            }
            return true;
        }
    return false;
}

bool Polygon::operator!=(const Shape& another) const
{
    return !(*this == another);
}

bool Polygon::isCongruentTo(const Shape& another) const
{
    if(!dynamic_cast<const Polygon*>(&another)) return false;
    const Polygon& p = *(const Polygon*)&another;

    Polygon pp = p;
    pp.translate(polygonCenter() - pp.polygonCenter());

    Point c = polygonCenter(), x = nearestVertex(), y = pp.nearestVertex();
    x = x - c;
    y = y - c;
    double dst = Point::distance(x, Point(0, 0));
    if(std::abs(dst - Point::distance(y, Point(0, 0))) >= ACCURACY) return false;
    double angle = acos((x.x * y.x + x.y * y.y) / (dst * dst)) * 180 / M_PI;
    if(isCongruentTo(pp, c, angle) || isCongruentTo(pp, c, -angle) || isCongruentTo(pp, c, 180 - angle) || isCongruentTo(pp, c, angle - 180))
        return true;

    pp.reflect(Line(polygonCenter(), 3));

    c = polygonCenter(), x = nearestVertex(), y = pp.nearestVertex();
    x = x - c;
    y = y - c;
    dst = Point::distance(x, Point(0, 0));
    if(std::abs(dst - Point::distance(y, Point(0, 0))) >= ACCURACY) return false;
    angle = acos((x.x * y.x + x.y * y.y) / (dst * dst)) * 180 / M_PI;
    if(isCongruentTo(pp, c, angle) || isCongruentTo(pp, c, -angle) || isCongruentTo(pp, c, 180 - angle) || isCongruentTo(pp, c, angle - 180))
        return true;
    return false;
}

bool Polygon::isSimilarTo(const Shape& another) const
{
    if(!dynamic_cast<const Polygon*>(&another)) return false;
    const Polygon& p = *(const Polygon*)&another;
    Polygon pp = p;
    pp.scale(pp.polygonCenter(), perimeter() / pp.perimeter());
    return isCongruentTo(pp);
}

bool Polygon::containsPoint(const Point& point) const
{
    bool res = false;
    size_t j = verticesCount() - 1;
//https://ru.stackoverflow.com/questions/464787/%D0%A2%D0%BE%D1%87%D0%BA%D0%B0-%D0%B2%D0%BD%D1%83%D1%82%D1%80%D0%B8-%D0%BC%D0%BD%D0%BE%D0%B3%D0%BE%D1%83%D0%B3%D0%BE%D0%BB%D1%8C%D0%BD%D0%B8%D0%BA%D0%B0
    for(size_t i = 0; i < verticesCount(); i++) {
        if(((vertices[i].y < point.y && vertices[j].y >= point.y) || 
            (vertices[j].y < point.y && vertices[i].y >= point.y)) &&
            (vertices[i].x + (point.y - vertices[i].y) / (vertices[j].y - vertices[i].y) * (vertices[j].x - vertices[i].x) < point.x))
            res = !res;
        j = i;
    }
    return res;
}


void Polygon::rotate(const Point& center, double angle)
{
    for(Point& v : vertices) v.rotate(center, angle);
}

void Polygon::reflect(const Point& center)
{
    for(Point& v : vertices) v.reflect(center);
}

void Polygon::reflect(const Line& axis)
{
    for(Point& v : vertices) v.reflect(axis);
}

void Polygon::scale(const Point& center, double coefficient)
{
    for(Point& v : vertices) v.scale(center, coefficient);
}


Point Polygon::polygonCenter() const
{
    Point res(0, 0);
    for(auto& v : getVertices()) res = res + v;
    return res / verticesCount();
}

Point Polygon::nearestVertex() const
{
    Point c = polygonCenter();
    Point min;
    double d = 3.40282347e+38, t;
    for(auto& v : getVertices()) {
        t = Point::distance(v, c);
        if(t < d) {
            d = t;
            min = v;
        }
    }
    return min;
}

void Polygon::translate(const Point& p)
{
    for(auto& v : vertices) v = v + p;
}

bool Polygon::isCongruentTo(const Polygon& p, const Point& c, double angle) const
{
    Polygon pp = p;
    pp.rotate(c, angle);
    return *this == pp;
}


Ellipse::Ellipse(const Point& f1, const Point& f2, double distances) : f1(f1), f2(f2), distances(distances)
{ }

std::pair<Point, Point> Ellipse::focuses() const
{
    return std::make_pair(f1, f2);
}

std::pair<Line, Line> Ellipse::directrices() const
{
    Line focuses = Line(f1, f2);
    Point cent = center();
    double dist = distances * distances / (2 * Point::distance(f1, f2));

    return std::make_pair(Line(focuses, cent, dist), Line(focuses, cent, -dist));
}

double Ellipse::eccentricity() const
{
    return Point::distance(f1, f2) / distances;
}

Point Ellipse::center() const
{
    return (f1 + f2) / 2;
}


double Ellipse::perimeter() const
{
    double e = eccentricity();
    double a = distances / 2;
    double b = a * std::sqrt(1 - e * e);
    double x = 3 * (a - b) / (a + b) * (a - b) / (a + b);
    return M_PI * (a + b) * (1 + x / (10 + std::sqrt(4 - x)));
}

double Ellipse::area() const
{
    double e = eccentricity();
    double a = distances / 2;
    double b = a * std::sqrt(1 - e * e);
    return M_PI * a * b;
}

bool Ellipse::operator==(const Shape& another) const
{
    if(!dynamic_cast<const Ellipse*>(&another)) return false;
    const Ellipse& e = *(const Ellipse*)&another;
    return std::abs(distances - e.distances) < ACCURACY && ((f1 == e.f1 && f2 == e.f2) || (f1 == e.f2 && f2 == e.f1));
}

bool Ellipse::operator!=(const Shape& another) const
{
    return !(*this == another);
}

bool Ellipse::isCongruentTo(const Shape& another) const
{
    if(!dynamic_cast<const Ellipse*>(&another)) return false;
    const Ellipse& e = *(const Ellipse*)&another;
    if(std::abs(distances - e.distances) >= ACCURACY) return false;
    return std::abs(Point::distance(f1, f2) - Point::distance(e.f1, e.f2)) < ACCURACY;
}

bool Ellipse::isSimilarTo(const Shape& another) const
{
    if(!dynamic_cast<const Ellipse*>(&another)) return false;
    const Ellipse& e = *(const Ellipse*)&another;
    double k = Point::distance(f1, f2) / Point::distance(e.f1, e.f2);
    return std::abs(k - (distances / e.distances)) < ACCURACY;
}

bool Ellipse::containsPoint(const Point& point) const
{
    return Point::distance(point, f1) + Point::distance(point, f2) < distances;
}


void Ellipse::rotate(const Point& center, double angle)
{
    f1.rotate(center, angle);
    f2.rotate(center, angle);
}

void Ellipse::reflect(const Point& center)
{
    f1.reflect(center);
    f2.reflect(center);
}

void Ellipse::reflect(const Line& axis)
{
    f1.reflect(axis);
    f2.reflect(axis);
}

void Ellipse::scale(const Point& center, double coefficient)
{
    f1.scale(center, coefficient);
    f2.scale(center, coefficient);
    distances *= coefficient;
}


Circle::Circle(const Point& center, double radius) : Ellipse(center, center, radius * 2)
{ }

double Circle::radius() const
{
    return distances / 2;
}


Rectangle::Rectangle(const Point& a, const Point& b, double ratio) : Polygon(a, b, b, Point())
{
    if(ratio < 1) ratio = 1 / ratio;
    double angle = std::atan(1 / ratio) * 180 / M_PI;
    
    vertices[1].rotate(a, -angle);
    vertices[1].scale(a, ratio / std::sqrt(1 + ratio * ratio));
    vertices[3] = vertices[1];
    vertices[3].reflect(Line(a, b));
}

Point Rectangle::center() const
{
    return (getVertices()[0] + getVertices()[2]) / 4;
}

std::pair<Line, Line> Rectangle::diagonals() const
{
    return std::make_pair(Line(getVertices()[0], getVertices()[2]), Line(getVertices()[1], getVertices()[3]));
}


Square::Square(const Point& a, const Point& b) : Rectangle(a, b, 1)
{ }

Circle Square::circumscribedCircle() const
{
    return Circle(center(), Point::distance(getVertices()[0], getVertices()[2]) / 2);
}

Circle Square::inscribedCircle() const
{
    return Circle(center(), Point::distance(getVertices()[0], getVertices()[1]) / 2);
}


Triangle::Triangle(const Point& a, const Point& b, const Point& c) : Polygon(a, b, c)
{ }

Circle Triangle::circumscribedCircle() const
{
    Line a(Line(vertices[0], vertices[1]), (vertices[0] + vertices[1]) / 2, 0);
    Line b(Line(vertices[1], vertices[2]), (vertices[1] + vertices[2]) / 2, 0);
    Point c = a & b;
    return Circle(c, Point::distance(c, vertices[0]));
}

Circle Triangle::inscribedCircle() const
{
    double a = Point::distance(vertices[1], vertices[2]);
    double b = Point::distance(vertices[0], vertices[2]);
    double c = Point::distance(vertices[0], vertices[1]);
    Line x(vertices[0], (vertices[2] * c + vertices[1] * b) / (b + c));
    Line y(vertices[1], (vertices[2] * c + vertices[0] * a) / (a + c));
    return Circle(x & y, 2 * area() / perimeter());
}

Point Triangle::centroid() const
{
    return polygonCenter();
}

Point Triangle::orthocenter() const
{
    Line a(Line(vertices[0], vertices[1]), vertices[2], 0);
    Line b(Line(vertices[1], vertices[2]), vertices[0], 0);
    return a & b;
}

Line Triangle::EulerLine() const
{
    return Line(orthocenter(), centroid());
}

Circle Triangle::ninePointsCircle() const
{
    return Triangle((vertices[0] + vertices[1]) / 2, (vertices[1] + vertices[2]) / 2, (vertices[0] + vertices[2]) / 2).circumscribedCircle();
}
