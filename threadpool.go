package threadpool

// #cgo CFLAGS: -I./include
// #cgo CXXFLAGS: -std=c++20 -I./include
// #include "thread_pool_wrapper.h"
import "C"
import (
	"errors"
	"runtime/cgo"
	"unsafe"
)

type ThreadPool struct {
	ptr unsafe.Pointer
}

//export goInvoke
func goInvoke(h C.uintptr_t) {
	cgoHandle := cgo.Handle(h)
	cgoHandle.Value().(func())()
	cgoHandle.Delete()
}

func NewThreadPool(nThreads, queueSize uint32) (*ThreadPool, error) {
	ptr := C.NewThreadPool(C.uint32_t(nThreads), C.uint32_t(queueSize))
	if ptr == nil {
		return nil, errors.New("failed to create threadpool")
	}
	return &ThreadPool{ptr: ptr}, nil
}

func (thp *ThreadPool) Submit(f func()) bool {
	h := cgo.NewHandle(f)
	return bool(C.Submit(thp.ptr, C.uintptr_t(h)))
}

func (thp *ThreadPool) Wait() {
	C.Wait(thp.ptr)
}

func (thp *ThreadPool) Destroy() {
	C.Destroy(thp.ptr)
}
