
#include "../header/cylinder_segmentation.h"

void cylinder_segmentation (std::vector<Cylinder> &cylinders, 
                  std::vector<pcl::PointCloud<pcl::PointXYZ>::Ptr > &clusters,
                  Eigen::Matrix4d T,
                  Parameters p)
{
  // Fit cylinders to the remainding clusters
  pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> ne;
  pcl::SACSegmentationFromNormals<pcl::PointXYZ, pcl::Normal> seg_cylinders; 
  pcl::ExtractIndices<pcl::PointXYZ> extract;

  // Allocate new memory to the cloud cylinder
  pcl::PointCloud<pcl::Normal>::Ptr cloud_normals (new pcl::PointCloud<pcl::Normal>);
  pcl::PointIndices::Ptr inliers_cylinder (new pcl::PointIndices);

  // Estimate point normals
  pcl::search::KdTree<pcl::PointXYZ>::Ptr tree (new pcl::search::KdTree<pcl::PointXYZ>);
  ne.setSearchMethod (tree);
  ne.setRadiusSearch (p.normal_radius_search); // Use neighbors in a sphere of radius 3cm
  // ne.setKSearch (50); // 50

  // Create the segmentation object for cylinder segmentation and set all the parameters
  seg_cylinders.setOptimizeCoefficients (true);
  seg_cylinders.setModelType (pcl::SACMODEL_CYLINDER);
  seg_cylinders.setMethodType (pcl::SAC_RANSAC);
  seg_cylinders.setNormalDistanceWeight (p.nd_weight); // 0.1
  seg_cylinders.setMaxIterations (10000);
  seg_cylinders.setDistanceThreshold (0.2); // 0.2
  seg_cylinders.setRadiusLimits (0.0, p.cylinder_max_radius); // 0.1
  seg_cylinders.setAxis (Eigen::Vector3f (0.0, 0.0, 1.0));
  seg_cylinders.setEpsAngle (pcl::deg2rad (15.0f));

  // Cylinder inliers and coefficients for each cluster 
for (std::vector<pcl::PointCloud<pcl::PointXYZ>::Ptr >::iterator 
                          it= clusters.begin(); it!=clusters.end(); ++it)
{
    // std::cout<< "Checking cluster: "<< std::distance(it, clusters.begin())
    //          << std::endl;

    // Allocate a new memory and extract there the cylinder cloud and coeffs
    pcl::ModelCoefficients::Ptr coefficients_cylinder (new pcl::ModelCoefficients);
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_cylinder 
                                  (new pcl::PointCloud<pcl::PointXYZ> ());

    // compute normals
    ne.setInputCloud (*it);
    ne.compute (*cloud_normals);

    // Segment cylinder
    seg_cylinders.setInputCloud (*it);
    seg_cylinders.setInputNormals (cloud_normals);
    seg_cylinders.segment (*inliers_cylinder, *coefficients_cylinder);

    // Extract indices
    extract.setInputCloud (*it);
    extract.setIndices (inliers_cylinder);
    extract.setNegative (false);

   
    extract.filter (*cloud_cylinder);

    if (cloud_cylinder->points.empty ())  // No cylinder extracted at all
    {
      // std::cout << "Can't find the cylindrical component from cluster " 
      //           << std::distance(clusters.begin(), it) << std::endl;
    }
    else if (cloud_cylinder->points.size() < p.min_cylinder_size) // too small cylinder
    {
      // cout<< "Cylinder from cluster "
      //          << std::distance(clusters.begin(), it) << " is too small"<< endl;
    }
    else // Good cylinder -> store in cylinders
    {

      // Obtain the cross zero axis point
      Eigen::Vector4d pose(0,0,0,1);
      Eigen::Vector4d axis(0,0,0,0);
      pose[0]= coefficients_cylinder->values[0];
      pose[1]= coefficients_cylinder->values[1];
      pose[2]= coefficients_cylinder->values[2];
      axis[0]= coefficients_cylinder->values[3];
      axis[1]= coefficients_cylinder->values[4];
      axis[2]= coefficients_cylinder->values[5];

      pose= T*pose;
      axis= T*axis;

      Eigen::Vector2d pose2x1;
      pose2x1[0]= pose[0] - pose[1]*(axis[0]/axis[1]); // x camera frame
      pose2x1[1]= pose[2] - pose[1]*(axis[2]/axis[1]); // z camera frame

      // Store the new cylinder
      Cylinder newCylinder;
      newCylinder.coefficients= coefficients_cylinder ;
      newCylinder.cloud= cloud_cylinder;
      newCylinder.pose= pose2x1;
      cylinders.push_back(newCylinder);
    }

    // cout<< "Finish checking cluster: "<< std::distance(clusters.begin(), it)<< endl;
  }


  // for (int j = 0; j < cylinders.coefficients.size(); ++j)
  // {
  //   cout<< "Cylinder "<< j<< " pose: \n"<< cylinders.pose[j]<< endl;
  // }

  // // Cylinders display
  // for (int i = 0; i < cylinders.cloud.size(); ++i)
  // {
  //   std::printf("Cylinder %d \t  #points  %lu \n",
  //                                      i, cylinders.cloud[i]->points.size());
  // }
  
}