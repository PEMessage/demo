package main

import (

	"github.com/gdamore/tcell/v2"
)

func main() {
	// 1. Initialize the screen
	screen, err := tcell.NewScreen()
	if err != nil {
		panic(err)
	}
	if err := screen.Init(); err != nil {
		panic(err)
	}
	defer screen.Fini()

	// 2. Clear the screen
	screen.Clear()

	// 3. Set up event handling
	events := make(chan tcell.Event)
	go func() {
		for {
			ev := screen.PollEvent()
			if ev == nil {
				close(events)
				return
			}
			events <- ev
		}
	}()

	// 4. Main loop
	for {
		// Draw content
		drawHelloWorld(screen)

		// Show the screen
		screen.Show()

		// Handle events
		select {
		case ev, ok := <-events:
			if !ok {
				return
			}
			switch ev := ev.(type) {
			case *tcell.EventKey:
				// Exit on ESC or Ctrl+C
				if ev.Key() == tcell.KeyEscape || ev.Key() == tcell.KeyCtrlC {
					return
				}
			case *tcell.EventResize:
				// Redraw on resize
				screen.Sync()
			}
		}
	}
}

func drawHelloWorld(screen tcell.Screen) {
	// Clear the screen
	screen.Clear()

	// Get terminal dimensions
	width, height := screen.Size()

	// Create a styled message
	style := tcell.StyleDefault.
		Foreground(tcell.ColorLightCyan).
		Bold(true)

	// Center the message
	msg := "Hello, Terminal UI!"
	x := (width - len(msg)) / 2
	y := height / 2

	// Write the message
	for i, ch := range msg {
		screen.SetContent(x+i, y, ch, nil, style)
	}
}

