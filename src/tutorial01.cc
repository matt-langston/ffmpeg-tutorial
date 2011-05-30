// tutorial01.c
// Code based on a tutorial by Martin Bohme (boehme@inb.uni-luebeckREMOVETHIS.de)
// Tested on Gentoo, CVS version 5/01/07 compiled with GCC 4.1.1

// A small sample program that shows how to use libavformat and libavcodec to
// read video from a file.
//
// Use
//
// gcc -o tutorial01 tutorial01.c -lavutil -lavformat -lavcodec -lz
//
// to build (assuming libavformat and libavcodec are correctly installed
// your system).
//
// Run using
//
// tutorial01 myvideofile.mpg
//
// to write the first five frames from "myvideofile.mpg" to disk in PPM
// format.

//#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}


static log4cxx::LoggerPtr logger( log4cxx::Logger::getLogger( "ffmpegtutorial.tutorial01" ) );


void SaveFrame( AVFrame*pFrame, int width, int height, int iFrame );


int main( int argc, char *argv[] ) {
  namespace fs = boost::filesystem;
  using boost::str;
  using boost::format;
  using boost::posix_time::seconds;
  //using namespace boost::posix_time;
  
  typedef fs::path path_t;
  //typedef boost::posix_time::time_duration time_duration_t;

  const path_t log4j_properties = "log4j.properties";
  if ( fs::exists( log4j_properties ) ) {
    // Default is 60 seconds. Units are milliseconds.
    //log4cxx::PropertyConfigurator::configureAndWatch( log4j_properties.string() );
    log4cxx::PropertyConfigurator::configureAndWatch( log4j_properties.string(), seconds(5).total_milliseconds() );
  } else {
    log4cxx::BasicConfigurator::configure();
  }
  
  if( argc < 2 ) {
    std::cerr << "Please provide a movie file\n";
    exit( EXIT_FAILURE );
  }

  const path_t input_file_path( argv[1] );
  if ( !fs::exists( input_file_path ) ) {
    std::cerr << str( format( "File %1% does not exist\n" ) % input_file_path );
    exit( EXIT_FAILURE );
  }
  
  // Register all formats and codecs.
  av_register_all();
  
  // Open video file.
  AVFormatContext* pFormatCtx;
  if( av_open_input_file( &pFormatCtx, input_file_path.string().c_str(), NULL, 0, NULL ) !=0 ) {
    LOG4CXX_ERROR( logger, str( format( "Could not open file \"%1%\"" ) % input_file_path ) );
    exit( EXIT_FAILURE );
  }
  
  // Retrieve stream information.
  if( av_find_stream_info( pFormatCtx ) < 0 ) {
    LOG4CXX_ERROR( logger, str( format( "Could not find stream information for file \"%1%\"" ) % input_file_path ) );
    exit( EXIT_FAILURE );
  }
  
  // Dump information about file onto standard error.
  dump_format( pFormatCtx, 0, input_file_path.string().c_str(), 0 );
  
  // Find the first video stream.
  int videoStream=-1;
  for( int i = 0; i < pFormatCtx -> nb_streams; ++i ) {
    if( pFormatCtx -> streams[i] -> codec -> codec_type == CODEC_TYPE_VIDEO ) {
      videoStream = i;
      LOG4CXX_DEBUG( logger, str( format( "Found video stream at index %1%" ) % videoStream ) );
      break;
    }
  }
  
  if( videoStream == -1 ) {
    LOG4CXX_ERROR( logger, str( format( "No video stream found for file \"%1%\"" ) % input_file_path ) );
    exit( EXIT_FAILURE );
  }
  
  // Get a pointer to the codec context for the video stream.
  AVCodecContext* pCodecCtx = pFormatCtx -> streams[videoStream] -> codec;
  
  // Find the decoder for the video stream.
  AVCodec* pCodec = avcodec_find_decoder( pCodecCtx -> codec_id );
  if( pCodec == NULL ) {
    LOG4CXX_ERROR( logger, str( format( "Unsupported video codec %1% for file \"%1%\"" ) % pCodecCtx -> codec_id % input_file_path ) );
    exit( EXIT_FAILURE );
  }
  
  // Open codec.
  if( avcodec_open( pCodecCtx, pCodec ) < 0 ) {
    LOG4CXX_ERROR( logger, str( format( "Could not opem video codec %1% for file \"%1%\"" ) % pCodecCtx -> codec_id % input_file_path ) );
    exit( EXIT_FAILURE );
  }
  
  // Allocate video frame.
  AVFrame* pFrame = avcodec_alloc_frame();
  if( pFrame == NULL ) {
    LOG4CXX_ERROR( logger, "Could not allocate AVFrame." );
    exit( EXIT_FAILURE );
  }
  
  // Allocate an AVFrame structure
  AVFrame* pFrameRGB = avcodec_alloc_frame();
  if( pFrameRGB == NULL ) {
    LOG4CXX_ERROR( logger, "Could not allocate AVFrame." );
    exit( EXIT_FAILURE );
  }

  // Determine required buffer size and allocate buffer.
  const PixelFormat pixel_format = PIX_FMT_RGB24;
  const int         width        = pCodecCtx -> width;
  const int         height       = pCodecCtx -> height;
  const int         numBytes     = avpicture_get_size( pixel_format, width, height );
  LOG4CXX_DEBUG( logger, str( format( "Frames of size %1% x %2% require %3% bytes" ) % width % height % numBytes ) );
  
  boost::uint8_t* buffer = static_cast<boost::uint8_t*>( av_malloc( numBytes * sizeof( boost::uint8_t ) ) );
  
  // Assign appropriate parts of buffer to image planes in pFrameRGB.
  // Note that pFrameRGB is an AVFrame, but AVFrame is a superset of
  // AVPicture.
  avpicture_fill( reinterpret_cast<AVPicture*>( pFrameRGB ), buffer, pixel_format, width, height );
  
  SwsContext* img_convert_ctx = sws_getContext( width, height, pCodecCtx -> pix_fmt, width, height, pixel_format, SWS_BICUBIC, NULL, NULL, NULL );
  if( img_convert_ctx == NULL ) {
    LOG4CXX_ERROR( logger, "Cannot initialize the conversion context." );
    exit( EXIT_FAILURE );
  }
  
  // Read frames and save first five frames to disk.
  AVPacket        packet;
  int             frameFinished = 0;
  std::size_t     num_frames    = 0;
  while( av_read_frame( pFormatCtx, &packet ) >= 0 ) {
    // Is this a packet from the video stream?
    if( packet.stream_index == videoStream ) {
      // Decode video frame
      //avcodec_decode_video( pCodecCtx, pFrame, &frameFinished, packet.data, packet.size );
      avcodec_decode_video2( pCodecCtx, pFrame, &frameFinished, &packet );
      
      // Did we get a video frame?
      if( frameFinished ) {
	// Convert the image from its native format to RGB.
	//LOG4CXX_DEBUG( logger, str( format( "Frame %1% has size %2% x %3%" ) % num_frames % pCodecCtx -> width % pCodecCtx -> height ) );
	//img_convert( reinterpret_cast<AVPicture*>( pFrameRGB ), pixel_format, reinterpret_cast<AVPicture*>( pFrame ), pCodecCtx->pix_fmt, width, height );
	sws_scale( img_convert_ctx, pFrame -> data, pFrame -> linesize, 0, height, pFrameRGB -> data, pFrameRGB -> linesize );
	
	// Save the frame to disk
	if( ++num_frames <= 5 ) {
	  SaveFrame( pFrameRGB, width, height, num_frames );
	}
      }
    }
    
    // Free the packet that was allocated by av_read_frame.
    av_free_packet( &packet );
  }
  
  // Free the RGB image.
  av_free( buffer    );
  av_free( pFrameRGB );
  
  // Free the YUV frame.
  av_free( pFrame );
  
  // Close the codec.
  avcodec_close( pCodecCtx );
  
  // Close the video file.
  av_close_input_file( pFormatCtx );
  
  return 0;
}


void SaveFrame( AVFrame* pFrame, int width, int height, int frame_number ) {
  namespace fs = boost::filesystem;
  using boost::str;
  using boost::format;
  
  typedef fs::path path_t;

  //const path_t output_file_path( str( format( "frame%1%.ppm" ) % frame_number ) );
  //fs::ofstream ofstream( output_file_path, std::ios::binary );
  
  // Open file
  char szFilename[32];
  sprintf( szFilename, "frame%d.ppm", frame_number );
  FILE* pFile = fopen( szFilename, "wb" );
  if( pFile == NULL ) {
    LOG4CXX_ERROR( logger, str( format( "Could not open file \"%1%\" for writing." ) % szFilename ) );
    return;
  }
  
  // Write header
  fprintf( pFile, "P6\n%d %d\n255\n", width, height );
  
  // Write pixel data
  for( int y = 0; y < height; ++y ) {
    fwrite( pFrame -> data[0] + y * pFrame -> linesize[0], 1, width * 3, pFile );
  }
  
  // Close file
  fclose(pFile);
}
