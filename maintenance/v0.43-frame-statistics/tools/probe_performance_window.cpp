#define main oracle_test_main
#include "../performance_window_tests.cpp"
#undef main
#include <chrono>
volatile double sink=0;
template<class Window> double measure(){
 Window w;std::uint32_t rng=48271;double sum=0;
 for(int i=0;i<300;++i){rng=rng*1664525u+1013904223u;w.add((rng%100000+1)*.000001);}
 const auto start=std::chrono::steady_clock::now();
 for(int i=0;i<200000;++i){rng=rng*1664525u+1013904223u;w.add((rng%100000+1)*.000001);const auto s=w.summary();sum+=s.fps+s.p95Ms+s.maxMs;}
 const auto end=std::chrono::steady_clock::now();sink=sum;
 return std::chrono::duration<double,std::milli>(end-start).count();
}
int main(){for(int i=0;i<5;++i){double old=measure<Reference>(),now=measure<timing::FrameWindow>();std::cout<<"200000 add+summary CPU synthetic random intervals: old_ms="<<old<<" new_ms="<<now<<"\n";}std::cout<<"Not Windows, HD6000, GPU, total-frame, or hardware FPS evidence.\n";}
