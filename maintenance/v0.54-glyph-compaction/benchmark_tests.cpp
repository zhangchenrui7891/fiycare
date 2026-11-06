#include "src/benchmark.h"
#include "src/run_files.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
void close(double a,double b,const char* why){require(std::abs(a-b)<1e-8,why);}
std::string report(const timing::BenchmarkSamples& samples){
 std::ostringstream out;timing::writeBenchmarkSummary(out,"v0.41","CPU_TEST_ONLY",true,false,samples);return out.str();
}
void noVerdict(const timing::BenchmarkSamples& samples){
 const auto text=report(samples);
 require(text.find("result_status=NOT_EVALUATED")!=std::string::npos,"Incomplete/invalid report has explicit status");
 require(text.find("avg_fps=NA\np95_ms=NA\nmax_ms=NA")!=std::string::npos,"No partial numeric frame statistics published");
 require(text.find("target_20=NOT_EVALUATED\ntarget_30=NOT_EVALUATED")!=std::string::npos,"No PASS or FAIL from invalid evidence");
}
void diagnosticRouting(){
 const std::filesystem::path base="PLAYER_DATA";
 for(bool benchmark:{false,true})for(bool smoke:{false,true}){
  const runfiles::RunFiles files(base,benchmark,smoke);
  require(files.folder()==(benchmark?base/"diagnostics"/"benchmark":smoke?base/"diagnostics"/"smoke":base),
          "Diagnostic path routing and benchmark precedence");
  require(files.gameSavesEnabled()==(!benchmark&&!smoke),"Only ordinary games allow career persistence");
 }
}
void reproduceLegacyClock(){
 // v0.35 add/elapsed excerpt. Virtual intervals, NOT measured GPU frames.
 struct Legacy {
  std::vector<double> frames;double total=0;bool overflow=false;
  void add(double seconds){
   if(!std::isfinite(seconds)||seconds<=0)return;
   if(frames.size()>=65536){overflow=true;return;}
   frames.push_back(seconds);total+=seconds;
  }
 } old;
 timing::BenchmarkSamples now;
 for(int i=0;i<200000;++i){old.add(.0005);now.add(.0005);}
 require(old.overflow&&old.total<60,"Legacy duration never reaches 60 after capacity hit");
 close(old.total,32.768,"Legacy clock frozen at sample capacity");close(now.elapsed(),100,"New duration keeps accepted time after capacity");
 require(now.storedSamples()==65536&&now.observedSamples()==200000&&now.discardedSamples()==134464,"Storage is bounded with transparent counts");
 require(!now.valid()&&now.summary().samples==0&&timing::benchmarkFinished(true,now),"Invalid run finishes without prefix statistics");
 noVerdict(now);
 std::cout<<"Legacy virtual dt=0.0005s x200000: elapsed freezes at "<<old.total
          <<"s; new accepted duration="<<now.elapsed()<<"s, storage="<<now.storedSamples()<<". Not hardware FPS.\n";
}
void capacityBoundaryAndEarlyEnd(){
 timing::BenchmarkSamples run;
 require(!timing::benchmarkFinished(false,run),"Unstarted benchmark does not finish");
 for(std::size_t i=0;i<timing::BenchmarkSamples::sampleLimit;++i)run.add(.0005);
 require(run.valid()&&run.summary().samples==65536&&!timing::benchmarkFinished(true,run),"Full but not overflowed capacity remains valid before 60s");
 const double before=run.elapsed();run.add(.0005);
 close(run.elapsed()-before,.0005,"Overflow interval still advances duration");
 require(run.overflowed()&&run.timingValid()&&!run.valid()&&run.discardedSamples()==1,"First excess sample invalidates evidence only");
 require(run.elapsed()<60&&timing::benchmarkFinished(true,run),"Overflow run ends early, not at a frozen 60s condition");
 require(!timing::benchmarkFinished(false,run),"Started flag remains required");
 noVerdict(run);
 std::cout<<"Capacity boundary: sample 65537 triggers early NOT_EVALUATED at "<<run.elapsed()<<" virtual seconds.\n";
}
void normalAndIncompleteRuns(){
 for(double dt:{.02,.04,.06}){
  timing::BenchmarkSamples run;while(!timing::benchmarkFinished(true,run))run.add(dt);
  const auto stats=run.summary();const auto result=timing::assessBenchmark(run.elapsed(),stats,run.valid());
  require(run.valid()&&result.complete&&run.elapsed()>=60&&stats.samples==run.observedSamples(),"Normal full run remains complete");
  close(stats.fps,1/dt,"Average uses all observed run intervals");close(stats.p95Ms,dt*1000,"Full-run P95 exact for constant intervals");
  require(result.target20==(dt<=.05)&&result.target30==(dt<.03334),"Existing target tiers preserved");
  const auto text=report(run);require(text.find("result_status=VALID")!=std::string::npos&&text.find("avg_fps=NA")==std::string::npos,"Valid report has numeric statistics");
 }
 timing::BenchmarkSamples slow;for(int i=0;i<60;++i)slow.add(1);
 require(slow.valid()&&timing::benchmarkFinished(true,slow),"Low-sample 60s run still ends");
 require(!timing::assessBenchmark(slow.elapsed(),slow.summary()).complete,"Insufficient samples not a performance failure");noVerdict(slow);
 timing::BenchmarkSamples stutter;for(int i=0;i<600;++i)stutter.add(.1);for(int i=0;i<300;++i)stutter.add(.02);
 require(stutter.summary().samples==900&&stutter.summary().p95Ms==100,"Early stutter remains in full-run statistics");
 std::cout<<"Normal target tiers/full-run P95 preserved; insufficient 60s evidence has no performance verdict.\n";
}
void malformedEvidence(){
 for(double bad:{0.,-1.,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}){
  timing::BenchmarkSamples run;run.add(.02);run.add(bad);
  require(!run.valid()&&!run.timingValid()&&!run.overflowed()&&run.rejectedIntervals()==1,"Bad interval invalidates run distinctly from capacity");
  close(run.elapsed(),.02,"Rejected interval does not enter duration");
  require(run.storedSamples()==1&&run.observedSamples()==1&&timing::benchmarkFinished(true,run),"Bad clock interval finishes early with unchanged sample count");noVerdict(run);
 }
 timing::BenchmarkSamples huge;huge.add(std::numeric_limits<double>::max());huge.add(std::numeric_limits<double>::max());
 require(!huge.valid()&&!huge.timingValid()&&std::isfinite(huge.elapsed()),"Accumulation overflow is finite and invalid");noVerdict(huge);
 const timing::FrameStats good{40,25,30,300};
 require(!timing::assessBenchmark(60,good,false).complete,"Explicit evidence flag cannot be bypassed by plausible statistics");
 for(const auto& stats:{timing::FrameStats{INFINITY,25,30,300},timing::FrameStats{40,NAN,30,300},
                       timing::FrameStats{40,25,INFINITY,300},timing::FrameStats{0,25,30,300},
                       timing::FrameStats{40,-1,30,300},timing::FrameStats{40,25,20,300}}){
  auto result=timing::assessBenchmark(60,stats);require(!result.complete&&!result.target20&&!result.target30,"Malformed statistics cannot produce a verdict");
 }
 require(!timing::assessBenchmark(INFINITY,good).complete&&!timing::assessBenchmark(NAN,good).complete,"Nonfinite elapsed is not complete evidence");
 std::cout<<"Invalid intervals, accumulation overflow, malformed stats and report NA/NOT_EVALUATED checks passed.\n";
}
void configurationChanges(){
 const timing::FrameContext base{3,1,1,0,true,true,true,false,false,640,360};
 for(int field=0;field<11;++field){
  timing::BenchmarkSamples run;run.observeContext(base);
  for(int i=0;i<3001;++i){run.observeContext(base);run.add(.02);}
  require(run.valid()&&timing::assessBenchmark(run.elapsed(),run.summary()).complete,"Stable context remains assessable");
  auto changed=base;
  switch(field){
   case 0:changed.aircraft=2;break;case 1:changed.camera=0;break;
   case 2:changed.terrain=0;break;case 3:changed.menu=1;break;
   case 4:changed.antiAlias=false;break;case 5:changed.low=false;break;
   case 6:changed.active=false;break;case 7:changed.walking=true;break;
   case 8:changed.powerSaver=true;break;case 9:changed.width=800;break;
   case 10:changed.height=600;break;
  }
  run.observeContext(changed);
  require(!run.valid()&&!run.configurationValid()&&run.timingValid()&&!run.overflowed(),"Changed configuration invalidates only its own evidence flag");
  require(timing::benchmarkFinished(true,run),"Changed run finishes instead of waiting for 60 seconds");
  noVerdict(run);
  require(report(run).find("configuration_valid=0")!=std::string::npos,"Report exposes configuration invalidation");
  run.observeContext(base);run.add(.02);noVerdict(run);
 }
 timing::BenchmarkSamples early;early.observeContext(base);auto changed=base;changed.camera=0;early.observeContext(changed);
 require(!timing::benchmarkFinished(false,early),"Unstarted guard retains existing start rule");
 early.add(.02);require(timing::benchmarkFinished(true,early),"First sampled invalid run ends early");noVerdict(early);
 std::cout<<"PASS 11 configuration mutations after valid full runs, sticky invalidation, early change, and report suppression. CPU only.\n";
}
int main(){
 try{configurationChanges();diagnosticRouting();reproduceLegacyClock();capacityBoundaryAndEarlyEnd();normalAndIncompleteRuns();malformedEvidence();
  std::cout<<"Benchmark CPU clock/report checks passed; no Windows, GPU FPS or HD6000 measurement performed.\n";
 }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
