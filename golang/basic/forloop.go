package main
import (
	"fmt"
	"time"
)


func main() {
	for i := 0 ; i < 5 ; i++ {
		<-time.After(time.Millisecond * 1000)
		fmt.Println("Hello")
	}
}
