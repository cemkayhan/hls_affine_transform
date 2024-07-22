#include "hls_vid_stabilization.h"

static void Func1(
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Height,
  hls::stream<int>& srcXStream,
  hls::stream<int>& srcYStream,
  hls::stream<int>& topLeftX,
  hls::stream<int>& topLeftY,
  float rotMat00, float rotMat01, float rotMat02,
  float rotMat10, float rotMat11, float rotMat12
){
#pragma HLS INLINE off

  loopRows: for(auto J_=0;J_<Height;J_+=D_BLOCK_SIZE_){
#pragma HLS LOOP_TRIPCOUNT min=D_MAX_ROWS_/D_BLOCK_SIZE_ max=D_MAX_ROWS_/D_BLOCK_SIZE_

    loopCols: for(auto K_=0;K_<Width;K_+=D_BLOCK_SIZE_){
#pragma HLS LOOP_TRIPCOUNT min=D_MAX_COLS_/D_BLOCK_SIZE_ max=D_MAX_COLS_/D_BLOCK_SIZE_

      auto topLeftXSet_{false};
      auto topLeftYSet_{false};
      int topLeftX_;
      int topLeftY_;

      loopBlockRows: for(auto JJ_=0;JJ_<D_BLOCK_SIZE_;++JJ_){
        loopBlockCols: for(auto KK_=0;KK_<D_BLOCK_SIZE_;++KK_){
#pragma HLS PIPELINE II=1

          const auto SrcX_{static_cast<int>(rotMat00*(KK_+K_)+rotMat01*(JJ_+J_)+rotMat02)};
          const auto SrcY_{static_cast<int>(rotMat10*(KK_+K_)+rotMat11*(JJ_+J_)+rotMat12)};
          srcXStream.write(SrcX_);
          srcYStream.write(SrcY_);
          if(!topLeftXSet_){
            topLeftXSet_=true;
            topLeftX_=SrcX_;
          } else if(topLeftX_>SrcX_){
            topLeftX_=SrcX_;
          }
          if(!topLeftYSet_){
            topLeftYSet_=true;
            topLeftY_=SrcY_;
          } else if(topLeftY_>SrcY_){
            topLeftY_=SrcY_;
          }
        }
      }
      topLeftX.write(topLeftX_);
      topLeftY.write(topLeftY_);
    }
  }
}

static void Func2(
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_MM_PPC_> srcAxi[(D_MAX_STRIDE_/D_MM_PPC_)*(2*D_MAX_ROWS_)],
  hls::stream<ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,1>::Value> >& srcStream,
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Height,
  hls::stream<int>& srcXStream,
  hls::stream<int>& srcYStream,
  hls::stream<int>& topLeftX,
  hls::stream<int>& topLeftY
){
#pragma HLS INLINE off

  loopRows: for(auto J_=0;J_<Height;J_+=D_BLOCK_SIZE_){
#pragma HLS LOOP_TRIPCOUNT min=D_MAX_ROWS_/D_BLOCK_SIZE_ max=D_MAX_ROWS_/D_BLOCK_SIZE_

    loopCols: for(auto K_=0;K_<Width;K_+=D_BLOCK_SIZE_){
//#pragma HLS LOOP_TRIPCOUNT min=D_MAX_COLS_/D_BLOCK_SIZE_ max=D_MAX_COLS_/D_BLOCK_SIZE_

      int topLeftX_;
      int topLeftY_;
      topLeftX>>topLeftX_;
      topLeftY>>topLeftY_;

      static ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,1>::Value> tmp[(2*D_BLOCK_SIZE_)][(2*D_BLOCK_SIZE_)];
#pragma HLS ARRAY_PARTITION variable=tmp type=cyclic factor=8 dim=2

#pragma HLS LOOP_MERGE

      loopBlockRows: for(auto JJ_=0;JJ_<(2*D_BLOCK_SIZE_);++JJ_){
        loopBlockCols: for(auto KK_=0;KK_<((2*D_BLOCK_SIZE_)/D_MM_PPC_);++KK_){
          const auto pix_ {srcAxi[(JJ_+topLeftY_+D_ROWS_MARGIN_)*(D_MAX_STRIDE_/D_MM_PPC_)+(KK_+((topLeftX_+D_COLS_MARGIN_)/D_MM_PPC_))]};
 
          loopBlockColsPpc: for(auto I_=0;I_<D_MM_PPC_;++I_){
#pragma HLS UNROLL
            tmp[JJ_][KK_*D_MM_PPC_+I_]=pix_(I_*D_COLOR_CHANNELS_*D_DEPTH_+D_COLOR_CHANNELS_*D_DEPTH_-1,I_*D_COLOR_CHANNELS_*D_DEPTH_);
          }
        }
      }

#if 4==D_MM_PPC_
      const ap_int<32> topLeftX2_ {topLeftX_};
      const auto tmpTmp2_ {ap_uint<2> {topLeftX2_(1,0)}};
#elif 2==D_MM_PPC_
      const ap_int<32> topLeftX2_ {topLeftX_};
      const auto tmpTmp2_ {ap_uint<2> {topLeftX2_(0,0)}};
#else
      const auto tmpTmp2_ {ap_uint<2> {0}};
#endif

      loopBlockRows2: for(auto JJ_=0;JJ_<D_BLOCK_SIZE_;++JJ_){
        loopBlockCols2: for(auto KK_=0;KK_<D_BLOCK_SIZE_;++KK_){
          int srcXStream_;
          int srcYStream_;
          srcXStream>>srcXStream_;
          srcYStream>>srcYStream_;
          srcStream<<tmp[(srcYStream_+D_ROWS_MARGIN_)-(topLeftY_+D_ROWS_MARGIN_)][(srcXStream_+tmpTmp2_+D_COLS_MARGIN_)-(topLeftX_+D_COLS_MARGIN_)];
        }
      }
    }
  }
}

static void Func3(
  hls::stream<ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,1>::Value> >& srcStream,
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_MM_PPC_> dstAxi[(D_MAX_STRIDE_/D_MM_PPC_)*D_MAX_ROWS_],
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> height
){
#pragma HLS INLINE off

  loopRows: for(auto J_=0;J_<height;J_+=D_BLOCK_SIZE_){
#pragma HLS LOOP_TRIPCOUNT min=D_MAX_ROWS_/D_BLOCK_SIZE_ max=D_MAX_ROWS_/D_BLOCK_SIZE_

    loopCols: for(auto K_=0;K_<width;K_+=D_BLOCK_SIZE_){
#pragma HLS LOOP_TRIPCOUNT min=D_MAX_COLS_/D_BLOCK_SIZE_ max=D_MAX_COLS_/D_BLOCK_SIZE_

      loopBlockRows: for(auto JJ_=0;JJ_<D_BLOCK_SIZE_;++JJ_){
        loopBlockCols: for(auto KK_=0;KK_<(D_BLOCK_SIZE_/D_MM_PPC_);++KK_){
          ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_MM_PPC_> dstAxiPix_;
          loopBlockColsPpc: for(auto I_=0;I_<D_MM_PPC_;++I_){
#pragma HLS UNROLL
            ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,1>::Value> srcStreamPix_;
            srcStream>>srcStreamPix_;
            dstAxiPix_(I_*D_COLOR_CHANNELS_*D_DEPTH_+D_COLOR_CHANNELS_*D_DEPTH_,I_*D_COLOR_CHANNELS_*D_DEPTH_)=srcStreamPix_;
          }
          dstAxi[(J_+JJ_)*(D_MAX_STRIDE_/D_MM_PPC_)+K_/D_MM_PPC_+KK_]=dstAxiPix_;
        }
      }
    }
  }
}

void D_TOP_
(
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_MM_PPC_> srcAxi[(D_MAX_STRIDE_/D_MM_PPC_)*(2*D_MAX_ROWS_)],
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_MM_PPC_> dstAxi[(D_MAX_STRIDE_/D_MM_PPC_)*D_MAX_ROWS_],
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> height,
  float rotMat00, float rotMat01, float rotMat02,
  float rotMat10, float rotMat11, float rotMat12
){
#pragma HLS INTERFACE m_axi port=srcAxi bundle=srcaxi depth=(D_MAX_STRIDE_/D_MM_PPC_)*(2*D_MAX_ROWS_)
#pragma HLS INTERFACE m_axi port=dstAxi bundle=dstaxi depth=(D_MAX_STRIDE_/D_MM_PPC_)*D_MAX_ROWS_

#pragma HLS INTERFACE s_axilite bundle=ctrl port=return
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x10 port=width
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x18 port=height
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x20 port=srcAxi
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x28 port=dstAxi
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x30 port=rotMat00
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x38 port=rotMat01
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x40 port=rotMat02
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x48 port=rotMat10
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x50 port=rotMat11
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x58 port=rotMat12

  const auto width_ {width};
  const auto height_ {height};

  const auto rotMat00_ {rotMat00};
  const auto rotMat01_ {rotMat01};
  const auto rotMat02_ {rotMat02};
  const auto rotMat10_ {rotMat10};
  const auto rotMat11_ {rotMat11};
  const auto rotMat12_ {rotMat12};

#pragma HLS DATAFLOW

  hls::stream<ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,1>::Value> > srcStream_("srcStream_");
#pragma HLS STREAM variable=srcStream_ depth=8

  hls::stream<int> topLeftX_("topLeftX");
#pragma HLS STREAM variable=topLeftX_ depth=8

  hls::stream<int> topLeftY_("topLeftY");
#pragma HLS STREAM variable=topLeftY_ depth=8

  hls::stream<int> srcXStream_("srcXStream_");
#pragma HLS STREAM variable=srcXStream_ depth=8

  hls::stream<int> srcYStream_("srcYStream_");
#pragma HLS STREAM variable=srcYStream_ depth=8

  Func1(width_,height_,srcXStream_,srcYStream_,topLeftX_,topLeftY_,rotMat00_,rotMat01_,rotMat02_,rotMat10_,rotMat11_,rotMat12_);
  Func2(srcAxi,srcStream_,width_,height_,srcXStream_,srcYStream_,topLeftX_,topLeftY_);
  Func3(srcStream_,dstAxi,width_,height_);
}
