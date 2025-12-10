package main
import (
	"fmt"
	"time"
)

func main() {

	c := make(chan bool)
	end := make(chan bool)

	go func() {
		fmt.Println("Send Start")
		c <- true // no buffer one, will be block
		fmt.Println("Send End")
		end <- true
	}()

	go func() {
		time.Sleep(time.Millisecond * 500)
		fmt.Println("Recv Start")
		<-c
		fmt.Println("Recv End")
		end <- true
	}()


	for i := 0 ; i < 2 ; i ++ {
		<-end
	}
}

