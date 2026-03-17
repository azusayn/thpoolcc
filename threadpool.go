package threadpool

// #cgo CFLAGS: -I./include
// #cgo CXXFLAGS: -std=c++20 -I./include
// #include "thread_pool_wrapper.h"
import "C"
import (
	"errors"
	"unsafe"
)

type ThreadPool struct {
	ptr unsafe.Pointer
}

func NewThreadPool(nThreads, queueSize uint32) (*ThreadPool, error) {
	thp := C.NewThreadPool(C.uint32_t(nThreads), C.uint32_t(queueSize))
	if thp == nil {
		return nil, errors.New("failed to create threadpool")
	}
	return &ThreadPool{ptr: thp}, nil
}
