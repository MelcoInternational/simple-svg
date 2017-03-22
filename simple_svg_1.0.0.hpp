
/*******************************************************************************
*  The "New BSD License" : http://www.opensource.org/licenses/bsd-license.php  *
********************************************************************************

Copyright (c) 2010, Mark Turney
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/

#ifndef SIMPLE_SVG_HPP
#define SIMPLE_SVG_HPP

#include <vector>
#include <string>
#include <sstream>
#include <fstream>

#include <iostream>

namespace svg {
    // Utility XML/String Functions.
    template<typename T>
    static std::string attribute(std::string const &attribute_name,
                                 T const &value, std::string const &unit = "") {
        std::stringstream ss;
        ss << attribute_name << "=\"" << value << unit << "\" ";
        return ss.str();
    }

    static std::string elemStart(std::string const &element_name) {
        return "\t<" + element_name + " ";
    }

    static std::string elemEnd(std::string const &element_name) {
        return "</" + element_name + ">\n";
    }

    static std::string emptyElemEnd() {
        return "/>\n";
    }

    // Quick optional return type.  This allows functions to return an invalid
    //  value if no good return is possible.  The user checks for validity
    //  before using the returned value.
    template<typename T>
    class optional {
    public:
        optional<T>(T const &type)
                : valid(true), type(type) {}

        optional<T>() : valid(false), type(T()) {}

        T *operator->() {
            // If we try to access an invalid value, an exception is thrown.
            if (!valid)
                throw std::exception();

            return &type;
        }

        // Test for validity.
        bool operator!() const { return !valid; }

    private:
        bool valid;
        T type;
    };

    struct Dimensions {
        Dimensions(double width, double height) : width(width), height(height) {}

        Dimensions(double combined = 0) : width(combined), height(combined) {}

        double width;
        double height;
    };

#ifndef SVG_POINT_TYPE
    struct Point
    {
        Point(double x = 0, double y = 0) : x(x), y(y) { }
        double x;
        double y;
    };
#else
    typedef SVG_POINT_TYPE Point;
#endif

    struct Rect {
        Point minPt = {0, 0}, maxPt = {0, 0};

        Rect() {}

        Rect(const Point &p, double w = 0, double h = 0) : minPt(p), maxPt(p.x + w, p.y + h) {}

        void include(const Rect &r) {
            include(r.minPt);
            include(r.maxPt);
        }

        double width() const {
            return maxPt.x - minPt.x;
        }

        double height() const {
            return maxPt.y - minPt.y;
        }

        void include(const Point &p) {
            if (p.x < minPt.x)
                minPt.x = p.x;
            if (p.y < minPt.y)
                minPt.y = p.y;
            if (p.x > maxPt.x)
                maxPt.x = p.x;
            if (p.y > maxPt.y)
                maxPt.y = p.y;
        }
    };

    static inline optional<Point> getMinPoint(std::vector<Point> const &points) {
        if (points.empty())
            return optional<Point>();

        Point min = points[0];
        for (unsigned i = 0; i < points.size(); ++i) {
            if (points[i].x < min.x)
                min.x = points[i].x;
            if (points[i].y < min.y)
                min.y = points[i].y;
        }
        return optional<Point>(min);
    }

    static inline optional<Point> getMaxPoint(std::vector<Point> const &points) {
        if (points.empty())
            return optional<Point>();

        Point max = points[0];
        for (unsigned i = 0; i < points.size(); ++i) {
            if (points[i].x > max.x)
                max.x = points[i].x;
            if (points[i].y > max.y)
                max.y = points[i].y;
        }
        return optional<Point>(max);
    }

    // Defines the dimensions, scale, origin, and origin offset of the document.
    struct Layout {
        enum Origin {
            TopLeft, BottomLeft, TopRight, BottomRight
        };

        Layout(Dimensions const &dimensions = Dimensions(400, 300), Origin origin = BottomLeft,
               double scale = 1, Point const &origin_offset = Point(0, 0))
                : dimensions(dimensions), scale(scale), origin(origin), origin_offset(origin_offset) {}

        Dimensions dimensions;
        double scale;
        Origin origin;
        Point origin_offset;
    };

    // Convert coordinates in user space to SVG native space.
    static inline double translateX(double x, Layout const &layout) {
        if (layout.origin == Layout::BottomRight || layout.origin == Layout::TopRight)
            return layout.dimensions.width - ((x + layout.origin_offset.x) * layout.scale);
        else
            return (layout.origin_offset.x + x) * layout.scale;
    }

    static inline double translateY(double y, Layout const &layout) {
        if (layout.origin == Layout::BottomLeft || layout.origin == Layout::BottomRight)
            return layout.dimensions.height - ((y + layout.origin_offset.y) * layout.scale);
        else
            return (layout.origin_offset.y + y) * layout.scale;
    }

    static inline double translateScale(double dimension, Layout const &layout) {
        return dimension * layout.scale;
    }

    class Serializeable {
    public:
        Serializeable() {}

        virtual ~Serializeable() {};

        virtual std::string toString() const = 0;
    };

    class Color : public Serializeable {
    public:
        enum Defaults {
            Transparent = -1, Aqua, Black, Blue, Brown, Cyan, Fuchsia,
            Green, Lime, Magenta, Orange, Purple, Red, Silver, White, Yellow
        };

        Color(int r, int g, int b) : transparent(false), red(r), green(g), blue(b) {}

        Color(Defaults color)
                : transparent(false), red(0), green(0), blue(0) {
            switch (color) {
                case Aqua:
                    assign(0, 255, 255);
                    break;
                case Black:
                    assign(0, 0, 0);
                    break;
                case Blue:
                    assign(0, 0, 255);
                    break;
                case Brown:
                    assign(165, 42, 42);
                    break;
                case Cyan:
                    assign(0, 255, 255);
                    break;
                case Fuchsia:
                    assign(255, 0, 255);
                    break;
                case Green:
                    assign(0, 128, 0);
                    break;
                case Lime:
                    assign(0, 255, 0);
                    break;
                case Magenta:
                    assign(255, 0, 255);
                    break;
                case Orange:
                    assign(255, 165, 0);
                    break;
                case Purple:
                    assign(128, 0, 128);
                    break;
                case Red:
                    assign(255, 0, 0);
                    break;
                case Silver:
                    assign(192, 192, 192);
                    break;
                case White:
                    assign(255, 255, 255);
                    break;
                case Yellow:
                    assign(255, 255, 0);
                    break;
                default:
                    transparent = true;
                    break;
            }
        }

        virtual ~Color() {}

        std::string toString() const {
            std::stringstream ss;
            if (transparent)
                ss << "transparent";
            else
                ss << "rgb(" << red << "," << green << "," << blue << ")";
            return ss.str();
        }

    private:
        bool transparent;
        int red;
        int green;
        int blue;

        void assign(int r, int g, int b) {
            red = r;
            green = g;
            blue = b;
        }
    };

    class Fill : public Serializeable {
    public:
        Fill(Color::Defaults color) : color(color) {}

        Fill(Color color = Color::Transparent)
                : color(color) {}

        std::string toString() const {
            std::stringstream ss;
            ss << attribute("fill", color.toString());
            return ss.str();
        }

    private:
        Color color;
    };

    class Stroke : public Serializeable {
    public:
        Stroke(double width = -1, Color color = Color::Transparent, bool nonScalingStroke = false)
                : width(width), color(color), nonScaling(nonScalingStroke) {}

        std::string toString() const {
            // If stroke width is invalid.
            if (width < 0)
                return std::string();

            std::stringstream ss;
            ss << attribute("stroke-width", width) << attribute("stroke", color.toString());
            if (nonScaling)
                ss << attribute("vector-effect", "non-scaling-stroke");
            return ss.str();
        }

    private:
        double width;
        Color color;
        bool nonScaling;
    };

    class Font : public Serializeable {
    public:
        Font(double size = 12, std::string const &family = "Verdana") : size(size), family(family) {}

        std::string toString() const {
            std::stringstream ss;
            ss << attribute("font-size", size) << attribute("font-family", family);
            return ss.str();
        }

    private:
        double size;
        std::string family;
    };

    class Shape : public Serializeable {
    public:
        Shape(Fill const &fill = Fill(), Stroke const &stroke = Stroke())
                : fill(fill), stroke(stroke) {}

        virtual ~Shape() {}

        virtual std::string toString() const = 0;

        virtual void offset(Point const &offset) = 0;

        virtual Rect MinMax() const = 0;

    protected:
        Fill fill;
        Stroke stroke;
    };

    template<typename T>
    std::string vectorToString(std::vector<T> collection, Layout const &layout) {
        std::string combination_str;
        for (unsigned i = 0; i < collection.size(); ++i)
            combination_str += collection[i].toString();

        return combination_str;
    }

    template<typename T>
    std::string vectorToString(std::vector<T> collection) {
        std::string combination_str;
        for (unsigned i = 0; i < collection.size(); ++i)
            combination_str += collection[i].toString();

        return combination_str;
    }

    class Circle : public Shape {
    public:
        Circle(Point const &center, double diameter, Fill const &fill,
               Stroke const &stroke = Stroke())
                : Shape(fill, stroke), center(center), radius(diameter / 2) {}

        std::string toString() const {
            std::stringstream ss;
            ss << elemStart("circle") << attribute("cx", center.x)
               << attribute("cy", center.y)
               << attribute("r", radius) << fill.toString()
               << stroke.toString() << emptyElemEnd();
            return ss.str();
        }

        void offset(Point const &offset) {
            center.x += offset.x;
            center.y += offset.y;
        }

        virtual Rect MinMax() const {
            return Rect(Point(center.x - radius, center.y - radius), radius * 2.0, radius * 2.0);
        };
    private:
        Point center;
        double radius;
    };

    class Elipse : public Shape {
    public:
        Elipse(Point const &center, double width, double height,
               Fill const &fill = Fill(), Stroke const &stroke = Stroke())
                : Shape(fill, stroke), center(center), radius_width(width / 2),
                  radius_height(height / 2) {}

        std::string toString() const {
            std::stringstream ss;
            ss << elemStart("ellipse") << attribute("cx", center.x)
               << attribute("cy", center.y)
               << attribute("rx", radius_width)
               << attribute("ry", radius_height)
               << fill.toString() << stroke.toString() << emptyElemEnd();
            return ss.str();
        }

        void offset(Point const &offset) {
            center.x += offset.x;
            center.y += offset.y;
        }

        virtual Rect MinMax() const {
            return Rect(Point(center.x - radius_width, center.y - radius_height), radius_width * 2, radius_height * 2);
        };
    private:
        Point center;
        double radius_width;
        double radius_height;
    };

    class Rectangle : public Shape {
    public:
        Rectangle(Point const &edge, double width, double height,
                  Fill const &fill = Fill(), Stroke const &stroke = Stroke())
                : Shape(fill, stroke), edge(edge), width(width),
                  height(height) {}

        std::string toString() const {
            std::stringstream ss;
            ss << elemStart("rect") << attribute("x", edge.x)
               << attribute("y", edge.y)
               << attribute("width", width)
               << attribute("height", height)
               << fill.toString() << stroke.toString() << emptyElemEnd();
            return ss.str();
        }

        void offset(Point const &offset) {
            edge.x += offset.x;
            edge.y += offset.y;
        }

        virtual Rect MinMax() const {
            return Rect(edge, width, height);
        };
    private:
        Point edge;
        double width;
        double height;
    };

    class Line : public Shape {
    public:
        Line(Point const &start_point, Point const &end_point,
             Stroke const &stroke = Stroke())
                : Shape(Fill(), stroke), start_point(start_point),
                  end_point(end_point) {}

        std::string toString() const {
            std::stringstream ss;
            ss << elemStart("line") << attribute("x1", start_point.x)
               << attribute("y1", start_point.y)
               << attribute("x2", end_point.x)
               << attribute("y2", end_point.y)
               << stroke.toString() << emptyElemEnd();
            return ss.str();
        }

        void offset(Point const &offset) {
            start_point.x += offset.x;
            start_point.y += offset.y;

            end_point.x += offset.x;
            end_point.y += offset.y;
        }

        virtual Rect MinMax() const {
            Rect rtn(start_point);
            rtn.include(end_point);
            return rtn;
        };
    private:
        Point start_point;
        Point end_point;
    };

    class Polygon : public Shape {
    public:
        Polygon(Fill const &fill = Fill(), Stroke const &stroke = Stroke())
                : Shape(fill, stroke) {}

        Polygon(Stroke const &stroke = Stroke()) : Shape(Color::Transparent, stroke) {}

        Polygon &operator<<(Point const &point) {
            points.push_back(point);
            return *this;
        }

        std::string toString() const {
            std::stringstream ss;
            ss << elemStart("polygon");

            ss << "points=\"";
            for (unsigned i = 0; i < points.size(); ++i)
                ss << points[i].x << "," << points[i].y << " ";
            ss << "\" ";

            ss << fill.toString() << stroke.toString() << emptyElemEnd();
            return ss.str();
        }

        void offset(Point const &offset) {
            for (unsigned i = 0; i < points.size(); ++i) {
                points[i].x += offset.x;
                points[i].y += offset.y;
            }
        }

        virtual Rect MinMax() const {
            if (points.empty())
                return Rect();

            Rect rtn(points.front());
            for (auto &pt : points)
                rtn.include(pt);
            return rtn;
        }

    private:
        std::vector<Point> points;
    };

    class Path : public Shape {
    public:
        Path(Fill const &fill = Fill(), Stroke const &stroke = Stroke())
                : Shape(fill, stroke) { startNewSubPath(); }

        Path(Stroke const &stroke = Stroke()) : Shape(Color::Transparent, stroke) { startNewSubPath(); }

        Path &operator<<(Point const &point) {
            paths.back().push_back(point);
            return *this;
        }

        void startNewSubPath() {
            if (paths.empty() || 0 < paths.back().size())
                paths.emplace_back();
        }

        std::string toString() const {
            std::stringstream ss;
            ss << elemStart("path");

            ss << "d=\"";
            for (auto const &subpath: paths) {
                if (subpath.empty())
                    continue;

                ss << "M";
                for (auto const &point: subpath)
                    ss << point.x << "," << point.y << " ";
                ss << "z ";
            }
            ss << "\" ";
            ss << "fill-rule=\"evenodd\" ";

            ss << fill.toString() << stroke.toString() << emptyElemEnd();
            return ss.str();
        }

        void offset(Point const &offset) {
            for (auto &subpath : paths)
                for (auto &point : subpath) {
                    point.x += offset.x;
                    point.y += offset.y;
                }
        }

        virtual Rect MinMax() const {
            if (paths.empty())
                return Rect();

            Rect rtn(paths.front().front());
            for (auto &path : paths) {
                for (auto &pt : path)
                    rtn.include(pt);
            }
            return rtn;
        };
    private:
        std::vector<std::vector<Point>> paths;
    };

    class Polyline : public Shape {
    public:
        Polyline(Fill const &fill = Fill(), Stroke const &stroke = Stroke())
                : Shape(fill, stroke) {}

        Polyline(Stroke const &stroke = Stroke()) : Shape(Color::Transparent, stroke) {}

        Polyline(std::vector<Point> const &points,
                 Fill const &fill = Fill(), Stroke const &stroke = Stroke())
                : Shape(fill, stroke), points(points) {}

        Polyline &operator<<(Point const &point) {
            points.push_back(point);
            return *this;
        }

        std::string toString() const {
            std::stringstream ss;
            ss << elemStart("polyline");

            ss << "points=\"";
            for (unsigned i = 0; i < points.size(); ++i)
                ss << points[i].x << "," << points[i].y << " ";
            ss << "\" ";

            ss << fill.toString() << stroke.toString() << emptyElemEnd();
            return ss.str();
        }

        void offset(Point const &offset) {
            for (unsigned i = 0; i < points.size(); ++i) {
                points[i].x += offset.x;
                points[i].y += offset.y;
            }
        }

        virtual Rect MinMax() const {
            if (points.empty())
                return Rect();

            Rect rtn(points.front());
            for (auto &pt : points)
                rtn.include(pt);
            return rtn;
        }

        std::vector<Point> points;
    };

    class Text : public Shape {
    public:
        Text(Point const &origin, std::string const &content, Fill const &fill = Fill(),
             Font const &font = Font(), Stroke const &stroke = Stroke())
                : Shape(fill, stroke), origin(origin), content(content), font(font) {}

        std::string toString() const {
            std::stringstream ss;
            ss << elemStart("text") << attribute("x", origin.x)
               << attribute("y", origin.y)
               << fill.toString() << stroke.toString() << font.toString()
               << ">" << content << elemEnd("text");
            return ss.str();
        }

        void offset(Point const &offset) {
            origin.x += offset.x;
            origin.y += offset.y;
        }

        virtual Rect MinMax() const {
            Rect rtn(origin);
            return rtn;
        }

    private:
        Point origin;
        std::string content;
        Font font;
    };

    // Sample charting class.
    class LineChart : public Shape {
    public:
        LineChart(Dimensions margin = Dimensions(), double scale = 1,
                  Stroke const &axis_stroke = Stroke(.5, Color::Purple))
                : axis_stroke(axis_stroke), margin(margin), scale(scale) {}

        LineChart &operator<<(Polyline const &polyline) {
            if (polyline.points.empty())
                return *this;

            polylines.push_back(polyline);
            return *this;
        }

        std::string toString() const {
            if (polylines.empty())
                return "";

            std::string ret;
            for (unsigned i = 0; i < polylines.size(); ++i)
                ret += polylineToString(polylines[i]);

            return ret + axisString();
        }

        void offset(Point const &offset) {
            for (unsigned i = 0; i < polylines.size(); ++i)
                polylines[i].offset(offset);
        }

        virtual Rect MinMax() const {
            Rect rtn;
            return rtn;
        }

    private:
        Stroke axis_stroke;
        Dimensions margin;
        double scale;
        std::vector<Polyline> polylines;

        optional<Dimensions> getDimensions() const {
            if (polylines.empty())
                return optional<Dimensions>();

            optional<Point> min = getMinPoint(polylines[0].points);
            optional<Point> max = getMaxPoint(polylines[0].points);
            for (unsigned i = 0; i < polylines.size(); ++i) {
                if (getMinPoint(polylines[i].points)->x < min->x)
                    min->x = getMinPoint(polylines[i].points)->x;
                if (getMinPoint(polylines[i].points)->y < min->y)
                    min->y = getMinPoint(polylines[i].points)->y;
                if (getMaxPoint(polylines[i].points)->x > max->x)
                    max->x = getMaxPoint(polylines[i].points)->x;
                if (getMaxPoint(polylines[i].points)->y > max->y)
                    max->y = getMaxPoint(polylines[i].points)->y;
            }

            return optional<Dimensions>(Dimensions(max->x - min->x, max->y - min->y));
        }

        std::string axisString(Layout const &layout) const {
            optional<Dimensions> dimensions = getDimensions();
            if (!dimensions)
                return "";

            // Make the axis 10% wider and higher than the data points.
            double width = dimensions->width * 1.1;
            double height = dimensions->height * 1.1;

            // Draw the axis.
            Polyline axis(Color::Transparent, axis_stroke);
            axis << Point(margin.width, margin.height + height) << Point(margin.width, margin.height)
                 << Point(margin.width + width, margin.height);

            return axis.toString();
        }

        std::string axisString() const {
            optional<Dimensions> dimensions = getDimensions();
            if (!dimensions)
                return "";

            // Make the axis 10% wider and higher than the data points.
            double width = dimensions->width * 1.1;
            double height = dimensions->height * 1.1;

            // Draw the axis.
            Polyline axis(Color::Transparent, axis_stroke);
            axis << Point(margin.width, margin.height + height) << Point(margin.width, margin.height)
                 << Point(margin.width + width, margin.height);

            return axis.toString();
        }

        std::string polylineToString(Polyline const &polyline) const {
            Polyline shifted_polyline = polyline;
            shifted_polyline.offset(Point(margin.width, margin.height));

            std::vector<Circle> vertices;
            for (unsigned i = 0; i < shifted_polyline.points.size(); ++i)
                vertices.push_back(Circle(shifted_polyline.points[i], getDimensions()->height / 30.0, Color::Black));

            return shifted_polyline.toString() + vectorToString(vertices);
        }

        std::string polylineToString(Polyline const &polyline, Layout const &layout) const {
            Polyline shifted_polyline = polyline;
            shifted_polyline.offset(Point(margin.width, margin.height));

            std::vector<Circle> vertices;
            for (unsigned i = 0; i < shifted_polyline.points.size(); ++i)
                vertices.push_back(Circle(shifted_polyline.points[i], getDimensions()->height / 30.0, Color::Black));

            return shifted_polyline.toString() + vectorToString(vertices, layout);
        }
    };

    class Document {
    public:
        Document(std::string const &file_name, Layout layout = Layout())
                : file_name(file_name), layout(layout) {}

        Rect region;

        Document &operator<<(Shape const &shape) {
            body_nodes_str += shape.toString();
            region.include(shape.MinMax());
            return *this;
        }

        std::string toString() const {

            std::stringstream ss;
            ss << "<?xml " << attribute("version", "1.0") << attribute("standalone", "no")
               << "?>\n<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" "
               << "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n<svg "
               << attribute("width", region.width(), "px")
               << attribute("height", region.height(), "px")
               << attribute("xmlns", "http://www.w3.org/2000/svg")
               << attribute("viewBox",
                            std::to_string(region.minPt.x) + " " +
                            std::to_string(region.minPt.y) + " " +
                            std::to_string(region.width()) + " " +
                            std::to_string(region.height()))
               << attribute("version", "1.1") << ">\n" << body_nodes_str << elemEnd("svg");
            return ss.str();
        }

        bool save() const {
            std::ofstream ofs(file_name.c_str());
            if (!ofs.good())
                return false;

            ofs << toString();
            ofs.close();
            return true;
        }

    private:
        std::string file_name;
        Layout layout;

        std::string body_nodes_str;
    };
}

#endif
