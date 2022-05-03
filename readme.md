 @file text2gds.cpp
  @author Ryan Miyakawa (rhmiyakawa@lbl.gov)
  @brief 
  @version 0.1
  @date 2022-05-03
  
  @copyright Copyright (c) 2022
  
  ### Usage
  ./text2gds input.csv output.gds
  
  ## Notes
  Reads a comma separated description of polygons and writes to GDS.  
  
  CSV should have coordinate pairs separated by commas with one row per polygons: x1,y1,x2,y2 ...
  Do not close boundaries; a redundant coordinate will be appended to end of list as per GDSII standard
  
  Coordinates should not have whitespace.
  
As per GDSII standards, polygon boundaries should not intersect or cross.
 https://www.iue.tuwien.ac.at/phd/minixhofer/node52.html
 
