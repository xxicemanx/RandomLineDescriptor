#include <opencv2/opencv.hpp>
#include <iostream>
#include <tuple>

using namespace cv;
using namespace std;

struct ProgramSettings {
   struct MserSettings {
      int delta;
      double minArea;
      double maxArea;
      double maxVariation;
      double minDiversity;
   };
   
   MserSettings mser;
};

void getEllipsePoints(vector<Point2d>& vertices, const RotatedRect& box, size_t points=1000U);
void drawEllipse(Mat& image, const RotatedRect& box, const Scalar& color = Scalar(0,0,255));
void drawEllipse(Mat& image, const vector<Point2d>& vertices, const Scalar& color = Scalar(0,0,255));
void genLines(vector<pair<Point2d,Point2d>>& pairs, const vector<Point2d>& vertices, double minDist=1.0, size_t pairCount=50);

void genLines(vector<pair<Point2d,Point2d>>& lines, const vector<Point2d>& vertices, double minDist, size_t pairCount)
{
   lines.resize(pairCount);

   for ( int i = 0; i < pairCount; ++i )
   {
      pair<Point2d,Point2d> p;

      int count = 0;
      do {
         p.first = vertices[rand() % vertices.size()];
         p.second = vertices[rand() % vertices.size()];
         count++;
      } while (sqrt((p.first.x-p.second.x)*(p.first.x-p.second.x)+
                    (p.first.y-p.second.y)*(p.first.y-p.second.y)) < minDist && count < 1000);

      // tried too many times, couldn't find a valid pair
      if ( count == 1000 )
      {
         cout << "Error: " << __LINE__ << ':' << __FILE__ << endl;
         return;
      }

      lines[i] = p;
   }
}

void getEllipsePoints(vector<Point2d>& vertices, const RotatedRect& box, size_t points)
{
   RotatedRect r = box;
  
   if ( r.size.height < r.size.width )
   {
      r.size.height = box.size.width;
      r.size.width = box.size.height;

      r.angle = (box.angle + 90.0);
   }
   
   r.angle *= M_PI / 180.0f;

   double a = r.size.height / 2.0;
   double b = r.size.width / 2.0;
   
   double step = 2 * M_PI / points;
   double theta = 0.0;

   double bCos, aSin, radius;

   vertices.resize(points);
   for ( size_t i = 0; i < points; ++i )
   {
      bCos = a * cos(theta);
      aSin = b * sin(theta);
      radius = a * b / sqrt(bCos * bCos + aSin * aSin);

      vertices[i] = Point2f(
         r.center.x + radius * cos(theta+r.angle),
         r.center.y + radius * sin(theta+r.angle));

      theta += step;
   }

}

void drawEllipse(Mat& image, const vector<Point2d>& vertices, const Scalar& color)
{
   for ( const Point2d& p : vertices )
      image.at<Vec3b>(p) = Vec3b(color[0],color[1],color[2]);
}

void drawEllipse(Mat& image, const RotatedRect& box, const Scalar& color)
{
   vector<Point2d> vertices(1000U);
   getEllipsePoints(vertices, box, 1000U);

   drawEllipse(image, vertices, color);

/*
   ellipse(image, box, Scalar(255,0,0));

   Point2f verts[4];
   box.points(verts);
   for ( int i = 0; i < 4; ++i )
      line(image, verts[i], verts[(i+1)%4], Scalar(255,0,0));
*/
}

void drawLines(Mat& image, const vector<pair<Point2d,Point2d>>& lines, const Scalar& color)
{
   for ( const pair<Point2d,Point2d>& p : lines )
   {
      line(image, p.first, p.second, color);
   }
}

int main(int argc, char *argv[])
{
   // image of black
   Mat img = imread(argv[1]);

   RotatedRect r(Point2f(250.0f, 250.0f), Size2f(400.0f,300.0f), 100);

   double threshold = min(r.size.width, r.size.height)/2.0;

   vector<Point2d> vertices(1000U);
   vector<pair<Point2d,Point2d>> lines;
   
   getEllipsePoints(vertices, r, 1000U);
   genLines(lines, vertices, threshold, 50);
   buildFeatures(image, lines);

   drawEllipse(img, vertices, Scalar(255,0,0));
   drawLines(img, lines, Scalar(255,255,255));

   imshow("Image", img);
   waitKey(0);

   return 0;

   if ( argc < 3 ) {
      cout << "Usage: " << argv[0] << " <input image> <output image>" << endl;
      return -1;
   }

   // TODO Load settings from file
   ProgramSettings settings;

   settings.mser.delta = 2;
   settings.mser.minArea = 0.00015;
   settings.mser.maxArea = 0.35;
   settings.mser.maxVariation = 0.25;
   settings.mser.minDiversity = 0.2;

   // images
   Mat image = imread(argv[1]);

   // convert to gray for MSER
   Mat grayImage;
   cvtColor(image, grayImage, CV_RGB2GRAY);

   // MSER from Dubout
   // delta = 2
   // minArea = 0.0001*imageArea
   // maxArea = 0.5*imageArea
   // maxVariation = 0.4
   // minDiversity = 0.33

   // MSER from OpenCV
   // delta = 5
   // minArea = 60
   // maxArea = 14400
   // maxVariation = 0.25
   // minDiversity = 0.2
   //    maxEvolution = 200
   //    areaThreshold=1.01
   //    minMargin = 0.003
   //    edgeBlurSize = 5

   // Construct an MSER detector
   MserFeatureDetector mserDetector(
      settings.mser.delta,
      settings.mser.minArea * image.size().area(),
      settings.mser.maxArea * image.size().area(),
      settings.mser.maxVariation,
      settings.mser.minDiversity);

   vector<vector<Point>> keypoints;

   mserDetector(grayImage, keypoints);

   for ( const vector<Point>& contour : keypoints ) {
      RotatedRect box = minAreaRect(contour);
//      ellipse( image, box, Scalar(255, 0, 0), 2 );
      drawEllipse( image, box, Scalar(255, 0, 0) );
   }

   imwrite(argv[2], image);

   waitKey(0);

   return 0;
}