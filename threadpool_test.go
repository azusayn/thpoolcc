package threadpool_test

import (
	"fmt"
	"testing"
	"time"

	"github.com/azusayn/threadpool"
)

func TestThreadpool(t *testing.T) {
	thp, err := threadpool.NewThreadPool(4, 8)
	if err != nil {
		t.Fatal(err.Error())
	}
	for range 5 {
		thp.Submit(func() {
			fmt.Println("excuted in goroutine")
			time.Sleep(time.Second * 1)
		})
	}
	thp.Wait()
	thp.Destroy()
}
