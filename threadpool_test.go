package threadpool_test

import (
	"testing"

	"github.com/azusayn/threadpool"
)

func TestNewThreadPool(t *testing.T) {
	if _, err := threadpool.NewThreadPool(1, 4); err != nil {
		t.Fatal(err.Error())
	}
	t.Log("success")
}
