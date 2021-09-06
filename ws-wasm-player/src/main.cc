#include <emscripten/bind.h>
#ifndef NDEBUG
#include <sanitizer/lsan_interface.h>
#endif

#include "log.h"
#include "frame.h"
#include "decoder.h"
#include "player.h"

using namespace emscripten;  // NOLINT

EMSCRIPTEN_BINDINGS(decoder) {
#ifndef NDEBUG
  function("DoLeakCheck", &__lsan_do_recoverable_leak_check);
#endif

  class_<Log>("Log")
    .class_function("set_prefix", &Log::set_prefix)
    .class_function("set_minlevel", &Log::set_minlevel)
    .class_function("set_v", &Log::set_v);

  class_<Frame>("Frame")
    .smart_ptr<std::shared_ptr<Frame>>("shared_ptr<Frame>")
    .property("type", &Frame::type)
    .property("data", &Frame::data_ptr)
    .property("linesize", &Frame::linesize)
    .property("width", &Frame::width)
    .property("height", &Frame::height)
    .property("format", &Frame::format)
    .property("pts", &Frame::pts)
    .property("size", &Frame::size)
    .function("getBytes", &Frame::GetBytes);

  class_<Decoder>("Decoder")
    .constructor<>()
    .function("open", &Decoder::Open)
    .function("decode", &Decoder::Decode, allow_raw_pointers());

  class_<OpenGLPlayer>("OpenGLPlayer")
    .smart_ptr_constructor("OpenGLPlayer", &std::make_shared<OpenGLPlayer>)
    .function("render", &OpenGLPlayer::Render);
}
