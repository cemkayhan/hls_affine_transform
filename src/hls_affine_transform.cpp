#include "hls_affine_transform.h"

template<int STRM_OUT_CHANNELS_,int DEPTH_,int STRM_OUT_PPC_,int MAX_COLS_,int MAX_ROWS_>
static void Func5(
  hls::stream<ap_uint<Axi_Vid_Bus_Width<STRM_OUT_CHANNELS_,DEPTH_,STRM_OUT_PPC_>::Value> >& dstLocalStream,
  hls::stream<ap_axiu<Axi_Vid_Bus_Width<STRM_OUT_CHANNELS_,DEPTH_,STRM_OUT_PPC_>::Value,1,1,1> >& dstStream,

  ap_uint<Bit_Width<MAX_COLS_>::Value> width,
  ap_uint<Bit_Width<MAX_ROWS_>::Value> height,

  ap_uint<Bit_Width<MAX_COLS_>::Value> padWidth,
  ap_uint<Bit_Width<MAX_ROWS_>::Value> padHeight
){
#pragma HLS INLINE OFF

  loopRows: for(auto J_=0;J_<padHeight;++J_)
#pragma HLS LOOP_TRIPCOUNT min=MAX_ROWS_ max=MAX_ROWS_

    loopCols: for(auto K_=0;K_<padWidth;K_+=STRM_OUT_PPC_){
#pragma HLS LOOP_TRIPCOUNT min=MAX_COLS_/STRM_OUT_PPC_ max=MAX_COLS_/STRM_OUT_PPC_
#pragma HLS PIPELINE II=1

    ap_uint<Axi_Vid_Bus_Width<STRM_OUT_CHANNELS_,DEPTH_,STRM_OUT_PPC_>::Value> dstLocalStreamPix_;
    dstLocalStream>>dstLocalStreamPix_;

    ap_axiu<Axi_Vid_Bus_Width<STRM_OUT_CHANNELS_,DEPTH_,STRM_OUT_PPC_>::Value,1,1,1> dstStreamPix_;
    dstStreamPix_.keep=-1;
    dstStreamPix_.strb=-1;
    dstStreamPix_.data=dstLocalStreamPix_;

    if(0==J_&&0==K_){
      dstStreamPix_.user=1;
      dstStreamPix_.last=0;
    } else if (K_==(padWidth-STRM_OUT_PPC_)) {
      dstStreamPix_.user=0;
      dstStreamPix_.last=1;
    } else {
      dstStreamPix_.user=0;
      dstStreamPix_.last=0;
    }
    
    if(J_<height&&K_<width){
      dstStream<<dstStreamPix_;
    }
  }
}

template<int DSTBRAM_DEPTH_,int MM_CHANNELS_,int STRM_OUT_CHANNELS_,int DEPTH_,int MM_PPC_,int MAX_ROWS_,int MAX_COLS_,int MAX_STRIDE_,int STRM_OUT_PPC_,int BLOCK_SIZE_>
static void Func4(
  ap_uint<MM_CHANNELS_*DEPTH_*MM_PPC_> dstBram[DSTBRAM_DEPTH_],
  hls::stream<bool>& dstBramTrigger,
  hls::stream<ap_uint<Axi_Vid_Bus_Width<STRM_OUT_CHANNELS_,DEPTH_,STRM_OUT_PPC_>::Value> >& dstStream,
  ap_uint<Bit_Width<MAX_COLS_>::Value> width,
  ap_uint<Bit_Width<MAX_ROWS_>::Value> height
){
#pragma HLS INLINE OFF

  loopRows: for(auto J_=0;J_<height;J_+=BLOCK_SIZE_){
#pragma HLS LOOP_TRIPCOUNT min=MAX_ROWS_/BLOCK_SIZE_ max=MAX_ROWS_/BLOCK_SIZE_

    bool dstBramTrigger_;
    dstBramTrigger>>dstBramTrigger_;

    loopBlockRows: for(auto JJ_=0;JJ_<BLOCK_SIZE_;++JJ_){
      loopCols: for(auto K_=0;K_<width;K_+=BLOCK_SIZE_){
#pragma HLS LOOP_TRIPCOUNT min=MAX_COLS_/BLOCK_SIZE_ max=MAX_COLS_/BLOCK_SIZE_

        loopBlockCols: for(auto KK_=0;KK_<(BLOCK_SIZE_/MM_PPC_);++KK_){
#pragma HLS PIPELINE II=MM_PPC_/STRM_OUT_PPC_

          ap_uint<MM_CHANNELS_*DEPTH_*MM_PPC_> dstBramPix_;
          dstBramPix_=dstBram[(JJ_+J_)*(MAX_STRIDE_/MM_PPC_)+(K_/MM_PPC_)+KK_];
          loopBlockColsPpc: for(auto II_=0;II_<(MM_PPC_/STRM_OUT_PPC_);++II_){
            ap_uint<Axi_Vid_Bus_Width<STRM_OUT_CHANNELS_,DEPTH_,STRM_OUT_PPC_>::Value> dstStreamPix_;
            ap_uint<MM_CHANNELS_*DEPTH_*STRM_OUT_PPC_> dstBramPix2_;
            dstBramPix2_=dstBramPix_(II_*STRM_OUT_PPC_*MM_CHANNELS_*DEPTH_+STRM_OUT_PPC_*MM_CHANNELS_*DEPTH_-1,II_*STRM_OUT_PPC_*MM_CHANNELS_*DEPTH_);
            for(auto T_=0;T_<STRM_OUT_PPC_;++T_){
              dstStreamPix_(T_*STRM_OUT_CHANNELS_*DEPTH_+STRM_OUT_CHANNELS_*DEPTH_-1,T_*STRM_OUT_CHANNELS_*DEPTH_)=dstBramPix2_(T_*MM_CHANNELS_*DEPTH_+STRM_OUT_CHANNELS_*DEPTH_-1,T_*MM_CHANNELS_*DEPTH_);
            }
            dstStream<<dstStreamPix_;
          }
        }
      }
    }
  }
}

template<int SRCAXI_DEPTH_,int MM_CHANNELS_,int STRM_IN_CHANNELS_,int DEPTH_,int STRM_IN_PPC_,int MM_PPC_,int MAX_ROWS_,int MAX_COLS_,int MAX_STRIDE_,int ROWS_MARGIN_,int COLS_MARGIN_>
static void Func0(
  hls::stream<ap_axiu<Axi_Vid_Bus_Width<STRM_IN_CHANNELS_,DEPTH_,STRM_IN_PPC_>::Value,1,1,1> >& srcStream,
  ap_uint<MM_CHANNELS_*DEPTH_*MM_PPC_> srcAxi[SRCAXI_DEPTH_],
  ap_uint<Bit_Width<MAX_COLS_>::Value> width,
  ap_uint<Bit_Width<MAX_ROWS_>::Value> height
){
#pragma HLS INLINE OFF

  loopRows: for(auto J_=0;J_<height;++J_){
#pragma HLS LOOP_TRIPCOUNT min=MAX_ROWS_ max=MAX_ROWS_

    loopCols: for(auto K_=0;K_<width/MM_PPC_;++K_){
#pragma HLS LOOP_TRIPCOUNT min=MAX_COLS_/MM_PPC_ max=MAX_COLS_/MM_PPC_
#pragma HLS PIPELINE II=MM_PPC_/STRM_IN_PPC_

      ap_uint<MM_CHANNELS_*DEPTH_*MM_PPC_> srcAxi_;
      loopPix: for(auto I_=0;I_<MM_PPC_/STRM_IN_PPC_;++I_){
        ap_axiu<Axi_Vid_Bus_Width<STRM_IN_CHANNELS_,DEPTH_,STRM_IN_PPC_>::Value,1,1,1> srcStreamPix_;
        srcStream>>srcStreamPix_;
        ap_uint<MM_CHANNELS_*DEPTH_*STRM_IN_PPC_> srcAxi2_;
        loopStrmInPpc: for(auto T_=0;T_<STRM_IN_PPC_;++T_){
          srcAxi2_(T_*MM_CHANNELS_*DEPTH_+STRM_IN_CHANNELS_*DEPTH_-1,T_*MM_CHANNELS_*DEPTH_)=srcStreamPix_.data(T_*STRM_IN_CHANNELS_*DEPTH_+STRM_IN_CHANNELS_*DEPTH_-1,T_*STRM_IN_CHANNELS_*DEPTH_);
        }
        srcAxi_(I_*STRM_IN_PPC_*MM_CHANNELS_*DEPTH_+STRM_IN_PPC_*MM_CHANNELS_*DEPTH_-1,I_*STRM_IN_PPC_*MM_CHANNELS_*DEPTH_)=srcAxi2_;
      }
      const auto index_ {(J_+ROWS_MARGIN_)*(MAX_STRIDE_/MM_PPC_)+(K_+COLS_MARGIN_/MM_PPC_)};
      assert((index_>=0 && index_<SRCAXI_DEPTH_) && "out of range error");
      srcAxi[index_]=srcAxi_;
    }
  }
}

template<typename FP_T_,int MAX_COLS_,int MAX_ROWS_,int STRM_INTR_PPC_,int BLOCK_SIZE_>
static void Func1(
  ap_uint<Bit_Width<MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<MAX_ROWS_>::Value> Height,
  hls::stream<ap_int<16*STRM_INTR_PPC_>>& srcXStream,
  hls::stream<ap_int<16*STRM_INTR_PPC_>>& srcYStream,
  hls::stream<ap_int<16>>& topLeftX,
  hls::stream<ap_int<16>>& topLeftY,
  ap_uint<Type_Width<FP_T_>::Value> rotMat00, ap_uint<Type_Width<FP_T_>::Value> rotMat01, ap_uint<Type_Width<FP_T_>::Value> rotMat02,
  ap_uint<Type_Width<FP_T_>::Value> rotMat10, ap_uint<Type_Width<FP_T_>::Value> rotMat11, ap_uint<Type_Width<FP_T_>::Value> rotMat12
){
#pragma HLS INLINE off

  loopRows: for(auto J_=0;J_<Height;J_+=BLOCK_SIZE_){
#pragma HLS LOOP_TRIPCOUNT min=MAX_ROWS_/BLOCK_SIZE_ max=MAX_ROWS_/BLOCK_SIZE_

    loopCols: for(auto K_=0;K_<Width;K_+=BLOCK_SIZE_){
#pragma HLS LOOP_TRIPCOUNT min=MAX_COLS_/BLOCK_SIZE_ max=MAX_COLS_/BLOCK_SIZE_

      auto topLeftXSet_{false};
      auto topLeftYSet_{false};
      ap_int<16> topLeftX_;
      ap_int<16> topLeftY_;

      loopBlockRows: for(auto JJ_=0;JJ_<BLOCK_SIZE_;++JJ_){
        loopBlockCols: for(auto KK_=0;KK_<(BLOCK_SIZE_/STRM_INTR_PPC_);++KK_){
          ap_int<16*STRM_INTR_PPC_> SrcX_;
          ap_int<16*STRM_INTR_PPC_> SrcY_;
          for(auto II_=0;II_<STRM_INTR_PPC_;++II_){
#pragma HLS PIPELINE II=1

            float fx_ = Fpt_Func(rotMat00)*((KK_*STRM_INTR_PPC_+II_)+K_)+Fpt_Func(rotMat01)*(JJ_+J_)+Fpt_Func(rotMat02);
            float fy_ = Fpt_Func(rotMat10)*((KK_*STRM_INTR_PPC_+II_)+K_)+Fpt_Func(rotMat11)*(JJ_+J_)+Fpt_Func(rotMat12);
            SrcX_(II_*16+15,II_*16)=static_cast<ap_int<16>>(Floor_Func(Fpt_Func(rotMat00)*((KK_*STRM_INTR_PPC_+II_)+K_)+Fpt_Func(rotMat01)*(JJ_+J_)+Fpt_Func(rotMat02)));
            SrcY_(II_*16+15,II_*16)=static_cast<ap_int<16>>(Floor_Func(Fpt_Func(rotMat10)*((KK_*STRM_INTR_PPC_+II_)+K_)+Fpt_Func(rotMat11)*(JJ_+J_)+Fpt_Func(rotMat12)));
            if(!topLeftXSet_){
              topLeftXSet_=true;
              topLeftX_=ap_int<16> {SrcX_(II_*16+15,II_*16)};
            } else if(topLeftX_>ap_int<16> {SrcX_(II_*16+15,II_*16)}){
              topLeftX_=ap_int<16> {SrcX_(II_*16+15,II_*16)};
            }
            if(!topLeftYSet_){
              topLeftYSet_=true;
              topLeftY_=ap_uint<16> {SrcY_(II_*16+15,II_*16)};
            } else if(topLeftY_>ap_int<16> {SrcY_(II_*16+15,II_*16)}){
              topLeftY_=ap_uint<16> {SrcY_(II_*16+15,II_*16)};
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

template<int SRCAXI_DEPTH_,int CHANNELS_,int DEPTH_,int MM_PPC_,int MAX_STRIDE_,int MAX_ROWS_,int STRM_INTR_PPC_,int MAX_COLS_,int BLOCK_SIZE_,int ROWS_MARGIN_,int COLS_MARGIN_>
static void Func2(
  ap_uint<CHANNELS_*DEPTH_*MM_PPC_> srcAxi[SRCAXI_DEPTH_],
  hls::stream<ap_uint<Axi_Vid_Bus_Width<CHANNELS_,DEPTH_,STRM_INTR_PPC_>::Value> >& srcStream,
  ap_uint<Bit_Width<MAX_COLS_>::Value> Width,
  ap_uint<Bit_Width<MAX_ROWS_>::Value> Height,
  hls::stream<ap_int<16*STRM_INTR_PPC_>>& srcXStream,
  hls::stream<ap_int<16*STRM_INTR_PPC_>>& srcYStream,
  hls::stream<ap_int<16>>& topLeftX,
  hls::stream<ap_int<16>>& topLeftY
){
#pragma HLS INLINE off

  loopRows: for(auto J_=0;J_<Height;J_+=BLOCK_SIZE_){
#pragma HLS LOOP_TRIPCOUNT min=MAX_ROWS_/BLOCK_SIZE_ max=MAX_ROWS_/BLOCK_SIZE_

    loopCols: for(auto K_=0;K_<Width;K_+=BLOCK_SIZE_){
#pragma HLS LOOP_TRIPCOUNT min=MAX_COLS_/BLOCK_SIZE_ max=MAX_COLS_/BLOCK_SIZE_

      ap_int<16> topLeftX_;
      ap_int<16> topLeftY_;
      topLeftX>>topLeftX_;
      topLeftY>>topLeftY_;
      ap_int<16> topLeftXAxi_;
      ap_int<16> topLeftYAxi_;
      if((topLeftY_+ROWS_MARGIN_)>=0&&topLeftY_<MAX_ROWS_&&(topLeftX_+COLS_MARGIN_)>=0&&topLeftX_<MAX_COLS_){
        topLeftXAxi_=topLeftX_;
        topLeftYAxi_=topLeftY_;
      } else {
        topLeftXAxi_=-COLS_MARGIN_;
        topLeftYAxi_=-ROWS_MARGIN_;
      }

      static ap_uint<Axi_Vid_Bus_Width<CHANNELS_,DEPTH_,1>::Value> tmp_[(2*BLOCK_SIZE_)][(2*BLOCK_SIZE_)];
#pragma HLS ARRAY_PARTITION variable=tmp_ type=cyclic factor=8 dim=2
#pragma HLS BIND_STORAGE variable=tmp_ type=RAM_T2P impl=URAM

      loopBlockRows: for(auto JJ_=0;JJ_<(2*BLOCK_SIZE_);++JJ_){
        loopBlockCols: for(auto KK_=0;KK_<((2*BLOCK_SIZE_)/MM_PPC_);++KK_){
#pragma HLS PIPELINE II=1

          const auto index_ {(JJ_+topLeftYAxi_+ROWS_MARGIN_)*(MAX_STRIDE_/MM_PPC_)+(KK_+((topLeftXAxi_+COLS_MARGIN_)/MM_PPC_))};
          assert((index_>=0 && index_<SRCAXI_DEPTH_) && "out of range error");
          const auto pix_=srcAxi[index_];
          loopBlockColsPpc: for(auto II_=0;II_<MM_PPC_;++II_){
            tmp_[JJ_][KK_*MM_PPC_+II_]=pix_(II_*CHANNELS_*DEPTH_+CHANNELS_*DEPTH_-1,II_*CHANNELS_*DEPTH_);
          }
        }
      }

      const auto tmpTmp2_ {(MM_PPC_>1) ? topLeftX_(Pow2<MM_PPC_>::Value-1,0) : 0};

      loopBlockRows2: for(auto JJ_=0;JJ_<BLOCK_SIZE_;++JJ_){
        loopBlockCols2: for(auto KK_=0;KK_<(BLOCK_SIZE_/STRM_INTR_PPC_);++KK_){
#pragma HLS PIPELINE II=1

          ap_uint<Axi_Vid_Bus_Width<CHANNELS_,DEPTH_,STRM_INTR_PPC_>::Value> srcStreamPix_;
          ap_int<16*STRM_INTR_PPC_> srcXStream_;
          ap_int<16*STRM_INTR_PPC_> srcYStream_;
          srcXStream>>srcXStream_;
          srcYStream>>srcYStream_;
          loopStrmPpc: for(auto II_=0;II_<STRM_INTR_PPC_;++II_){
            if((topLeftY_+ROWS_MARGIN_)>=0&&topLeftY_<MAX_ROWS_&&(topLeftX_+COLS_MARGIN_)>=0&&topLeftX_<MAX_COLS_){
              const auto index1_ {(ap_int<16> {srcYStream_(II_*16+15,II_*16)}+ROWS_MARGIN_)-(topLeftY_+ROWS_MARGIN_)};
              assert((index1_>=0 && index1_<(2*BLOCK_SIZE_)) && "out of range error");
              const auto index2_ {(ap_int<16> {srcXStream_(II_*16+15,II_*16)}+tmpTmp2_+COLS_MARGIN_)-(topLeftX_+COLS_MARGIN_)};
              assert((index2_>=0 && index2_<(2*BLOCK_SIZE_)) && "out of range error");
              ap_uint<Axi_Vid_Bus_Width<CHANNELS_,DEPTH_,1>::Value> tmpPix_;
              srcStreamPix_(II_*CHANNELS_*DEPTH_+CHANNELS_*DEPTH_-1,II_*CHANNELS_*DEPTH_)=tmp_[index1_][index2_];
            } else {
              srcStreamPix_(II_*CHANNELS_*DEPTH_+CHANNELS_*DEPTH_-1,II_*CHANNELS_*DEPTH_)=0x0;
            }
          }
          srcStream<<srcStreamPix_;
        }
      }
    }
  }
}

template<int DSTBRAM_DEPTH_,int CHANNELS_,int DEPTH_,int MM_PPC_,int STRM_INTR_PPC_,int MAX_ROWS_,int MAX_COLS_,int MAX_STRIDE_,int BLOCK_SIZE_>
static void Func3(
  hls::stream<ap_uint<Axi_Vid_Bus_Width<CHANNELS_,DEPTH_,STRM_INTR_PPC_>::Value> >& srcStream,
  ap_uint<CHANNELS_*DEPTH_*MM_PPC_> dstBram[DSTBRAM_DEPTH_],
  hls::stream<bool>& dstBramTrigger,
  ap_uint<Bit_Width<MAX_COLS_>::Value> width,
  ap_uint<Bit_Width<MAX_ROWS_>::Value> height
){
#pragma HLS INLINE off

  loopRows: for(auto J_=0;J_<height;J_+=BLOCK_SIZE_){
#pragma HLS LOOP_TRIPCOUNT min=MAX_ROWS_/BLOCK_SIZE_ max=MAX_ROWS_/BLOCK_SIZE_

    loopCols: for(auto K_=0;K_<width;K_+=BLOCK_SIZE_){
#pragma HLS LOOP_TRIPCOUNT min=MAX_COLS_/BLOCK_SIZE_ max=MAX_COLS_/BLOCK_SIZE_

      loopBlockRows: for(auto JJ_=0;JJ_<BLOCK_SIZE_;++JJ_){
        loopBlockCols: for(auto KK_=0;KK_<(BLOCK_SIZE_/MM_PPC_);++KK_){
#pragma HLS PIPELINE II=MM_PPC_/STRM_INTR_PPC_
 
          ap_uint<CHANNELS_*DEPTH_*MM_PPC_> dstBramPix_;
          loopBlockColsPpc: for(auto II_=0;II_<(MM_PPC_/STRM_INTR_PPC_);++II_){
            ap_uint<Axi_Vid_Bus_Width<CHANNELS_,DEPTH_,STRM_INTR_PPC_>::Value> srcStreamPix_;
            srcStream>>srcStreamPix_;
            ap_uint<CHANNELS_*DEPTH_*STRM_INTR_PPC_> dstBramPix2_;
            loopStrmInPpc: for(auto T_=0;T_<STRM_INTR_PPC_;++T_){
              dstBramPix2_(T_*CHANNELS_*DEPTH_+CHANNELS_*DEPTH_-1,T_*CHANNELS_*DEPTH_)=srcStreamPix_(T_*CHANNELS_*DEPTH_+CHANNELS_*DEPTH_-1,T_*CHANNELS_*DEPTH_);
            }
            dstBramPix_(II_*STRM_INTR_PPC_*CHANNELS_*DEPTH_+STRM_INTR_PPC_*CHANNELS_*DEPTH_-1,II_*STRM_INTR_PPC_*CHANNELS_*DEPTH_)=dstBramPix2_;
          }
          const auto index_ {(J_+JJ_)*(MAX_STRIDE_/MM_PPC_)+(K_/MM_PPC_)+KK_};
          assert((index_>=0 && index_<DSTBRAM_DEPTH_) && "out of range error");
          dstBram[index_]=dstBramPix_;
        }
      }
    }
    dstBramTrigger.write(true);
  }
}

void D_TOP_
(
  ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> vidWrAxi[D_VIDWRAXI_DEPTH_],
  ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> affRdAxi[D_AFFRDAXI_DEPTH_],
  ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> affWrAxi[D_AFFWRAXI_DEPTH_],
  ap_uint<D_MM_CHANNELS_*D_DEPTH_*D_MM_PPC_> vidRdAxi[D_VIDRDAXI_DEPTH_],

  hls::stream<ap_axiu<Axi_Vid_Bus_Width<D_STRM_IN_CHANNELS_,D_DEPTH_,D_STRM_IN_PPC_>::Value,1,1,1> >& srcStream,
  hls::stream<ap_axiu<Axi_Vid_Bus_Width<D_STRM_OUT_CHANNELS_,D_DEPTH_,D_STRM_OUT_PPC_>::Value,1,1,1> >& dstStream,

  ap_uint<Bit_Width<D_MAX_COLS_>::Value> width,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> height,

  ap_uint<Bit_Width<D_MAX_COLS_>::Value> padWidth,
  ap_uint<Bit_Width<D_MAX_ROWS_>::Value> padHeight,

  ap_uint<Type_Width<D_FP_T_>::Value> rotMat00, ap_uint<Type_Width<D_FP_T_>::Value> rotMat01, ap_uint<Type_Width<D_FP_T_>::Value> rotMat02,
  ap_uint<Type_Width<D_FP_T_>::Value> rotMat10, ap_uint<Type_Width<D_FP_T_>::Value> rotMat11, ap_uint<Type_Width<D_FP_T_>::Value> rotMat12
){
#pragma HLS INTERFACE m_axi port=vidWrAxi latency=D_VIDWRAXI_LATENCY_ offset=slave bundle=vidwraxibnd num_write_outstanding=D_VIDWRAXI_NUM_WRITE_OUTSTANDING_ num_read_outstanding=1  max_write_burst_length=16 max_read_burst_length=2  depth=D_VIDWRAXI_DEPTH_
#pragma HLS INTERFACE m_axi port=affRdAxi latency=D_AFFRDAXI_LATENCY_ offset=slave bundle=affrdaxibnd num_read_outstanding=D_AFFRDAXI_NUM_READ_OUTSTANDING_   num_write_outstanding=1 max_read_burst_length=16  max_write_burst_length=2 depth=D_AFFRDAXI_DEPTH_
#pragma HLS INTERFACE m_axi port=affWrAxi latency=D_AFFWRAXI_LATENCY_ offset=slave bundle=affwraxibnd num_write_outstanding=D_AFFWRAXI_NUM_WRITE_OUTSTANDING_ num_read_outstanding=1  max_write_burst_length=16 max_read_burst_length=2  depth=D_AFFWRAXI_DEPTH_
#pragma HLS INTERFACE m_axi port=vidRdAxi latency=D_VIDRDAXI_LATENCY_ offset=slave bundle=vidrdaxibnd num_read_outstanding=D_VIDRDAXI_NUM_READ_OUTSTANDING_   num_write_outstanding=1 max_read_burst_length=16  max_write_burst_length=2 depth=D_VIDRDAXI_DEPTH_

#pragma HLS INTERFACE axis port=srcStream
#pragma HLS INTERFACE axis port=dstStream

#pragma HLS INTERFACE s_axilite bundle=ctrl port=return
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x10 port=width
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x18 port=height
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x20 port=padWidth
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x28 port=padHeight
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x30 port=vidWrAxi
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x38 port=affRdAxi
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x40 port=affWrAxi
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x48 port=vidRdAxi
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x50 port=rotMat00
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x58 port=rotMat01
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x60 port=rotMat02
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x68 port=rotMat10
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x70 port=rotMat11
#pragma HLS INTERFACE s_axilite bundle=ctrl offset=0x78 port=rotMat12

  const auto width_ {width};
  const auto height_ {height};

  const auto padWidth_ {padWidth};
  const auto padHeight_ {padHeight};

  const auto rotMat00_ {rotMat00};
  const auto rotMat01_ {rotMat01};
  const auto rotMat02_ {rotMat02};
  const auto rotMat10_ {rotMat10};
  const auto rotMat11_ {rotMat11};
  const auto rotMat12_ {rotMat12};

#pragma HLS DATAFLOW

  hls::stream<ap_uint<Axi_Vid_Bus_Width<D_MM_CHANNELS_,D_DEPTH_,D_STRM_INTR_PPC_>::Value> > localStream_("localStream");
#pragma HLS STREAM variable=localStream_ depth=2*D_BLOCK_SIZE_*(D_BLOCK_SIZE_/D_STRM_INTR_PPC_)

  hls::stream<ap_uint<Axi_Vid_Bus_Width<D_STRM_OUT_CHANNELS_,D_DEPTH_,D_STRM_OUT_PPC_>::Value> > dstLocalStream_("dstLocalStream");
#pragma HLS STREAM variable=dstLocalStream_ depth=2*D_BLOCK_SIZE_*(D_BLOCK_SIZE_/D_STRM_OUT_PPC_)

  hls::stream<ap_int<16*D_STRM_INTR_PPC_>> srcXStream_("srcXStream");
#pragma HLS STREAM variable=srcXStream_ depth=2*D_BLOCK_SIZE_*(D_BLOCK_SIZE_/D_STRM_INTR_PPC_)

  hls::stream<ap_int<16*D_STRM_INTR_PPC_>> srcYStream_("srcYStream");
#pragma HLS STREAM variable=srcYStream_ depth=2*D_BLOCK_SIZE_*(D_BLOCK_SIZE_/D_STRM_INTR_PPC_)

  hls::stream<ap_int<16>> topLeftX_("topLeftX");
#pragma HLS STREAM variable=topLeftX_ depth=8

  hls::stream<ap_int<16>> topLeftY_("topLeftY");
#pragma HLS STREAM variable=topLeftY_ depth=8

  hls::stream<bool> dstBramTrigger_("dstBramTrigger");
#pragma HLS STREAM variable=dstBramTrigger_ depth=8

  //
  Func0<D_VIDWRAXI_DEPTH_,D_MM_CHANNELS_,D_STRM_IN_CHANNELS_,D_DEPTH_,D_STRM_IN_PPC_,D_MM_PPC_,D_MAX_ROWS_,D_MAX_COLS_,D_MAX_STRIDE_,D_ROWS_MARGIN_,D_COLS_MARGIN_>(
    srcStream,
    vidWrAxi,
    width_,
    height_
  );

  //
  Func1<D_FP_T_,D_MAX_COLS_,D_MAX_ROWS_,D_STRM_INTR_PPC_,D_BLOCK_SIZE_>(
    padWidth_,
    padHeight_,
    srcXStream_,
    srcYStream_,
    topLeftX_,
    topLeftY_,
    rotMat00_,
    rotMat01_,
    rotMat02_,
    rotMat10_,
    rotMat11_,
    rotMat12_
  );

  //
  Func2<D_AFFRDAXI_DEPTH_,D_MM_CHANNELS_,D_DEPTH_,D_MM_PPC_,D_MAX_STRIDE_,D_MAX_ROWS_,D_STRM_INTR_PPC_,D_MAX_COLS_,D_BLOCK_SIZE_,D_ROWS_MARGIN_,D_COLS_MARGIN_>(
    affRdAxi,
    localStream_,
    padWidth_,
    padHeight_,
    srcXStream_,
    srcYStream_,
    topLeftX_,
    topLeftY_
  );

  //
  Func3<D_AFFWRAXI_DEPTH_,D_MM_CHANNELS_,D_DEPTH_,D_MM_PPC_,D_STRM_INTR_PPC_,D_MAX_ROWS_,D_MAX_COLS_,D_MAX_STRIDE_,D_BLOCK_SIZE_>(
    localStream_,
    affWrAxi,
    dstBramTrigger_,
    padWidth_,
    padHeight_
  );

  //
  Func4<D_VIDRDAXI_DEPTH_,D_MM_CHANNELS_,D_STRM_OUT_CHANNELS_,D_DEPTH_,D_MM_PPC_,D_MAX_ROWS_,D_MAX_COLS_,D_MAX_STRIDE_,D_STRM_OUT_PPC_,D_BLOCK_SIZE_>(
    vidRdAxi,
    dstBramTrigger_,
    dstLocalStream_,
    padWidth_,
    padHeight_
  );

  //
  Func5<D_STRM_OUT_CHANNELS_,D_DEPTH_,D_STRM_OUT_PPC_,D_MAX_ROWS_,D_MAX_COLS_>(
    dstLocalStream_,
    dstStream,
    width_,
    height_,
    padWidth_,
    padHeight_
  );
}
