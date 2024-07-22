#include "hls_vid_stabilization.h"

static void Func1(
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Height,
  hls::stream<ap_int<16*D_STRM_PPC_>>& srcXStream,
  hls::stream<ap_int<16*D_STRM_PPC_>>& srcYStream,
  hls::stream<ap_int<16>>& topLeftX,
  hls::stream<ap_int<16>>& topLeftY,
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
      ap_int<16> topLeftX_;
      ap_int<16> topLeftY_;

      loopBlockRows: for(auto JJ_=0;JJ_<D_BLOCK_SIZE_;++JJ_){
        loopBlockCols: for(auto KK_=0;KK_<(D_BLOCK_SIZE_/D_STRM_PPC_);++KK_){
          ap_int<16*D_STRM_PPC_> SrcX_;
          ap_int<16*D_STRM_PPC_> SrcY_;
          for(auto II_=0;II_<D_STRM_PPC_;++II_){
#pragma HLS PIPELINE II=1
            SrcX_(II_*16+15,II_*16)=static_cast<ap_int<16>>(rotMat00*((KK_*D_STRM_PPC_+II_)+K_)+rotMat01*(JJ_+J_)+rotMat02);
            SrcY_(II_*16+15,II_*16)=static_cast<ap_int<16>>(rotMat10*((KK_*D_STRM_PPC_+II_)+K_)+rotMat11*(JJ_+J_)+rotMat12);
            if(!topLeftXSet_){
              topLeftXSet_=true;
              topLeftX_=SrcX_(II_*16+15,II_*16);
            } else if(topLeftX_>ap_int<16> {SrcX_(II_*16+15,II_*16)}){
              topLeftX_=SrcX_(II_*16+15,II_*16);
            }
            if(!topLeftYSet_){
              topLeftYSet_=true;
              topLeftY_=SrcY_(II_*16+15,II_*16);
            } else if(topLeftY_>ap_int<16> {SrcY_(II_*16+15,II_*16)}){
              topLeftY_=SrcY_(II_*16+15,II_*16);
            }
          }
          srcXStream.write(SrcX_);
          srcYStream.write(SrcY_);
        }
      }
      topLeftX.write(topLeftX_);
      topLeftY.write(topLeftY_);
    }
  }
}

static void Func2(
  ap_uint<D_COLOR_CHANNELS_*D_DEPTH_*D_MM_PPC_> srcAxi[(D_MAX_STRIDE_/D_MM_PPC_)*(2*D_MAX_ROWS_)],
  hls::stream<ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_STRM_PPC_>::Value> >& srcStream,
  ap_uint<Bit_Width<D_MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> Height,
  hls::stream<ap_int<16*D_STRM_PPC_>>& srcXStream,
  hls::stream<ap_int<16*D_STRM_PPC_>>& srcYStream,
  hls::stream<ap_int<16>>& topLeftX,
  hls::stream<ap_int<16>>& topLeftY
){
#pragma HLS INLINE off

  loopRows: for(auto J_=0;J_<Height;J_+=D_BLOCK_SIZE_){
#pragma HLS LOOP_TRIPCOUNT min=D_MAX_ROWS_/D_BLOCK_SIZE_ max=D_MAX_ROWS_/D_BLOCK_SIZE_

    loopCols: for(auto K_=0;K_<Width;K_+=D_BLOCK_SIZE_){
    //loopCols: for(auto K_=0;K_<D_MAX_COLS_;K_+=D_BLOCK_SIZE_){
#pragma HLS LOOP_TRIPCOUNT min=D_MAX_COLS_/D_BLOCK_SIZE_ max=D_MAX_COLS_/D_BLOCK_SIZE_

      ap_int<16> topLeftX_;
      ap_int<16> topLeftY_;
      topLeftX>>topLeftX_;
      topLeftY>>topLeftY_;

      static ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,1>::Value> tmp[D_STRM_PPC_][(2*D_BLOCK_SIZE_)][(2*D_BLOCK_SIZE_)];
#pragma HLS ARRAY_PARTITION variable=tmp type=block factor=4 dim=1
#pragma HLS BIND_STORAGE variable=tmp type=RAM_T2P impl=URAM

//#pragma HLS LOOP_MERGE

      loopBlockRows: for(auto JJ_=0;JJ_<(2*D_BLOCK_SIZE_);++JJ_){
        loopBlockCols: for(auto KK_=0;KK_<((2*D_BLOCK_SIZE_)/D_MM_PPC_);++KK_){
          const auto pix_ {srcAxi[(JJ_+topLeftY_+D_ROWS_MARGIN_)*(D_MAX_STRIDE_/D_MM_PPC_)+(KK_+((topLeftX_+D_COLS_MARGIN_)/D_MM_PPC_))]};
 
          loopBlockColsPpc: for(auto II_=0;II_<D_MM_PPC_;++II_){
#pragma HLS UNROLL
            tmp[II_][JJ_][KK_*D_MM_PPC_+II_]=pix_(II_*D_COLOR_CHANNELS_*D_DEPTH_+D_COLOR_CHANNELS_*D_DEPTH_-1,II_*D_COLOR_CHANNELS_*D_DEPTH_);
          }
        }
      }

#if 4==D_MM_PPC_
      const ap_int<16> topLeftX2_ {topLeftX_};
      const auto tmpTmp2_ {ap_uint<2> {topLeftX2_(1,0)}};
#elif 2==D_MM_PPC_
      const ap_int<16> topLeftX2_ {topLeftX_};
      const auto tmpTmp2_ {ap_uint<2> {topLeftX2_(0,0)}};
#else
      const auto tmpTmp2_ {ap_uint<2> {0}};
#endif

      loopBlockRows2: for(auto JJ_=0;JJ_<D_BLOCK_SIZE_;++JJ_){
        loopBlockCols2: for(auto KK_=0;KK_<(D_BLOCK_SIZE_/D_STRM_PPC_);++KK_){
          ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_STRM_PPC_>::Value> srcStreamPix_;
          ap_int<16*D_STRM_PPC_> srcXStream_;
          ap_int<16*D_STRM_PPC_> srcYStream_;
          srcXStream>>srcXStream_;
          srcYStream>>srcYStream_;
          loopStrmPpc: for(auto II_=0;II_<D_STRM_PPC_;++II_){
#pragma HLS UNROLL
            srcStreamPix_(II_*D_COLOR_CHANNELS_*D_DEPTH_+D_COLOR_CHANNELS_*D_DEPTH_-1,II_*D_COLOR_CHANNELS_*D_DEPTH_)=tmp[II_][(ap_int<16> {srcYStream_(II_*16+15,II_*16)}+D_ROWS_MARGIN_)-(topLeftY_+D_ROWS_MARGIN_)][(ap_int<16> {srcXStream_(II_*16+15,II_*16)}+tmpTmp2_+D_COLS_MARGIN_)-(topLeftX_+D_COLS_MARGIN_)];
          }
          srcStream<<srcStreamPix_;
        }
      }
    }
  }
}

static void Func3(
  hls::stream<ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_STRM_PPC_>::Value> >& srcStream,
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
          loopBlockColsPpc: for(auto II_=0;II_<(D_MM_PPC_/D_STRM_PPC_);++II_){
            ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_STRM_PPC_>::Value> srcStreamPix_;
            srcStream>>srcStreamPix_;
            dstAxiPix_(II_*D_STRM_PPC_*D_COLOR_CHANNELS_*D_DEPTH_+D_STRM_PPC_*D_COLOR_CHANNELS_*D_DEPTH_-1,II_*D_STRM_PPC_*D_COLOR_CHANNELS_*D_DEPTH_)=srcStreamPix_;
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

  hls::stream<ap_uint<Axi_Vid_Bus_Width<D_COLOR_CHANNELS_,D_DEPTH_,D_STRM_PPC_>::Value> > srcStream_("srcStream_");
#pragma HLS STREAM variable=srcStream_ depth=8

  hls::stream<ap_int<16>> topLeftX_("topLeftX");
#pragma HLS STREAM variable=topLeftX_ depth=8

  hls::stream<ap_int<16>> topLeftY_("topLeftY");
#pragma HLS STREAM variable=topLeftY_ depth=8

  hls::stream<ap_int<16*D_STRM_PPC_>> srcXStream_("srcXStream_");
#pragma HLS STREAM variable=srcXStream_ depth=8

  hls::stream<ap_int<16*D_STRM_PPC_>> srcYStream_("srcYStream_");
#pragma HLS STREAM variable=srcYStream_ depth=8

  Func1(width_,height_,srcXStream_,srcYStream_,topLeftX_,topLeftY_,rotMat00_,rotMat01_,rotMat02_,rotMat10_,rotMat11_,rotMat12_);
  Func2(srcAxi,srcStream_,width_,height_,srcXStream_,srcYStream_,topLeftX_,topLeftY_);
  Func3(srcStream_,dstAxi,width_,height_);
}
