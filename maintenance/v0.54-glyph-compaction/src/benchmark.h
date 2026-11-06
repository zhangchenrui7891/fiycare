#pragma once
#include "performance.h"
#include <cmath>
#include <limits>
#include <ostream>
#include <string>
#include <vector>

namespace timing {
// Bounded full-run samples, separate from the 300-frame HUD window.
class BenchmarkSamples {
 std::vector<double> frames;
 double total=0;
 std::size_t observed=0,rejected=0;
 bool overflow=false,invalidTime=false,configurationChanged=false,contextObserved=false;
 FrameContext initialContext{};
public:
 static constexpr std::size_t sampleLimit=65536;
 // Sticky invalidation: returning to the original configuration cannot repair
 // a run that already contains different workloads.
 void observeContext(const FrameContext& context){
  if(!contextObserved){initialContext=context;contextObserved=true;}
  else if(!(initialContext==context))configurationChanged=true;
 }
 bool configurationValid()const{return !configurationChanged;}
 void add(double seconds){
  if(!std::isfinite(seconds)||seconds<=0){++rejected;invalidTime=true;return;}
  ++observed;
  const double next=total+seconds;
  if(!std::isfinite(next)){invalidTime=true;total=std::numeric_limits<double>::max();}
  else total=next;
  // Duration is independent of storage. A capacity hit must not freeze the
  // run clock before 60s; the driver also ends invalid runs immediately.
  if(frames.size()>=sampleLimit){overflow=true;return;}
  frames.push_back(seconds);
 }
 double elapsed()const{return total;}
 bool valid()const{return !overflow&&!invalidTime&&!configurationChanged;}
 bool overflowed()const{return overflow;}
 bool timingValid()const{return !invalidTime;}
 std::size_t storedSamples()const{return frames.size();}
 std::size_t observedSamples()const{return observed;}
 std::size_t discardedSamples()const{return observed-frames.size();}
 std::size_t rejectedIntervals()const{return rejected;}
 FrameStats summary()const{
  FrameStats s;if(!valid()||frames.empty())return s;s.samples=frames.size();
  auto sorted=frames;std::sort(sorted.begin(),sorted.end());
  s.fps=frames.size()/total;s.p95Ms=sorted[(95*frames.size()+99)/100-1]*1000;s.maxMs=sorted.back()*1000;return s;
 }
};
inline bool benchmarkFinished(bool started,const BenchmarkSamples& samples){
 return started&&(!samples.valid()||samples.elapsed()>=60.0);
}
struct BenchmarkResult {
 bool complete=false;
 bool target20=false;
 bool target30=false;
 std::string recommendation;
};

inline BenchmarkResult assessBenchmark(double elapsed,const FrameStats& stats,bool evidenceValid=true){
 BenchmarkResult r;
 if(!evidenceValid||!std::isfinite(elapsed)||elapsed<0){
  r.recommendation="基准样本或计时无效：本次不作性能判定。";return r;
 }
 if(elapsed<60.0||stats.samples<120){r.recommendation="样本不足：保持窗口前台运行满60秒。";return r;}
 if(!std::isfinite(stats.fps)||!std::isfinite(stats.p95Ms)||!std::isfinite(stats.maxMs)||
    stats.fps<=0||stats.p95Ms<=0||stats.maxMs<stats.p95Ms){
  r.recommendation="帧统计无效：本次不作性能判定。";return r;
 }
 r.complete=true;
 // Require average throughput and the 95th-percentile interval. This is not
 // a worst-frame guarantee: intervals beyond P95 may still be slower.
 r.target20=stats.fps>=20.0&&stats.p95Ms<=50.0;
 r.target30=stats.fps>=30.0&&stats.p95Ms<=33.34;
 if(r.target30)r.recommendation="达到30 FPS目标，可逐项提高地景或光影。";
 else if(r.target20)r.recommendation="达到低配20 FPS目标；保持当前设置。";
 else r.recommendation="未达到20 FPS目标：先关闭FXAA，再用DX11低负载模式复测。";
 return r;
}
inline BenchmarkResult writeBenchmarkSummary(std::ostream& out,const std::string& version,
 const std::string& backend,bool fxaa,bool powerSaver,const BenchmarkSamples& samples){
 const auto stats=samples.summary();const auto result=assessBenchmark(samples.elapsed(),stats,samples.valid());
 out<<"FlightCareerAir "<<version<<" fixed benchmark\nbackend="<<backend
    <<"\ninitial_scene=PVG,A350,cockpit,noon,low\nscene_assumption=continuous_scene_not_locked\nconfiguration_guard=FrameContext_11_fields\nfxaa="<<(fxaa?"on":"off")
    <<"\nsoftware_frame_cap="<<(powerSaver?20:0)<<"\nrender_size=640x360\nsample_scope=full_active_run\ntiming_source=CPU_wall_frame_intervals_not_GPU_timestamps"
    <<"\nsample_limit="<<BenchmarkSamples::sampleLimit<<"\nsample_overflow="<<samples.overflowed()
    <<"\nconfiguration_valid="<<samples.configurationValid()
    <<"\ntiming_valid="<<samples.timingValid()<<"\nsamples_valid="<<samples.valid()
    <<"\nelapsed_s="<<samples.elapsed()<<"\nsamples="<<stats.samples
    <<"\nsamples_stored="<<samples.storedSamples()<<"\nsamples_observed="<<samples.observedSamples()
    <<"\nsamples_discarded="<<samples.discardedSamples()<<"\nrejected_intervals="<<samples.rejectedIntervals()
    <<"\nresult_status="<<(result.complete?"VALID":"NOT_EVALUATED");
 if(result.complete)out<<"\navg_fps="<<stats.fps<<"\np95_ms="<<stats.p95Ms<<"\nmax_ms="<<stats.maxMs;
 else out<<"\navg_fps=NA\np95_ms=NA\nmax_ms=NA";
 out<<"\ntarget_20="<<(result.complete?(result.target20?"PASS":"FAIL"):"NOT_EVALUATED")
    <<"\ntarget_30="<<(result.complete?(result.target30?"PASS":"FAIL"):"NOT_EVALUATED")
    <<"\nrecommendation="<<result.recommendation<<'\n';
 return result;
}
}
