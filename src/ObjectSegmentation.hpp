#ifndef _CBR_OBJECT_SEGMENTATION_HPP_
#define _CBR_OBJECT_SEGMENTATION_HPP_



//object segmenter h file

namespace CBR
{

  class ObjectSegmentation
  {

  private:

  protected:
    Trace* mTrace;
    
  public:
    virtual ServerID lookup(const UUID& obj_id);
    virtual void osegChangeMessage(OSegChangeMessage*) = 0;
    virtual void tick(const Time& t) = 0;

  };
}
#endif
