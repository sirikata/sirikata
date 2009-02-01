namespace Sirikata {
class Noncopyable{
   protected:
      Noncopyable(){
      }
      ~Noncopyable() {
      }
   private:
      Noncopyable( const Noncopyable& );
      const Noncopyable& operator=( const Noncopyable& );
};
}
