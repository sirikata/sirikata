#ifndef _MESHDATA_HPP_

extern long Meshdata_counter;

struct Meshdata {
    std::vector<Sirikata::Vector3f> positions;
    std::vector<Sirikata::Vector3f> normals;
    std::vector<Sirikata::Vector2f> texUVs;
    std::vector<int> position_indices;
    std::vector<int> normal_indices;
    std::string texture;
    std::string uri;
    int up_axis;
    long id;
   
    Meshdata() {
        id=Meshdata_counter++;
        std::cout << "dbm debug Meshdata ctor id: " << id << "\n";
    }
};

#define _MESHDATA_HPP_ true
#endif
