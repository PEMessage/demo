package main

import (
	"fmt"
	"os"
	"reflect"
	"strconv"
)

type Config struct {
	Host string `env:"APP_HOST"`
	Port int    `env:"APP_PORT"`
}

// ===== explainRt: demonstrates common methods of reflect.Type =====
func explainRt(rt reflect.Type) {
	fmt.Println("== reflect.Type (rt) ==")                                    // "== reflect.Type (rt) =="
	fmt.Printf("Kind       = %v\n", rt.Kind())                                // Kind       = struct

	fmt.Printf("PkgPath    = %q\n", rt.PkgPath())                             // PkgPath    = "main"
	fmt.Printf("Name       = %q\n", rt.Name())                                // Name       = "Config"
	fmt.Printf("String     = %q\n", rt.String())                              // String     = "main.Config"

	fmt.Printf("Size       = %d bytes\n", rt.Size())                          // Size       = 24 bytes   (amd64: string=16 + int=8)

	fmt.Printf("Comparable = %v\n", rt.Comparable())                          // Comparable = true

	fmt.Printf("NumMethod  = %d\n", rt.NumMethod())                           // NumMethod  = 0
	fmt.Printf("NumField   = %d\n", rt.NumField())                            // NumField   = 2
	for i := 0; i < rt.NumField(); i++ {
		f := rt.Field(i) // StructField{Name, Type, Tag, Offset, Index}
		fmt.Printf("Field(%d)   = Name:%q Type:%v Tag:%q Offset:%d\n",
			i, f.Name, f.Type, string(f.Tag), f.Offset)
		// Field(0)   = Name:"Host" Type:string Tag:"env:\"APP_HOST\"" Offset:0
		// Field(1)   = Name:"Port" Type:int Tag:"env:\"APP_PORT\"" Offset:16
	}
	if f, ok := rt.FieldByName("Host"); ok {
		fmt.Printf("FieldByName(\"Host\").Tag.Get(\"env\") = %q\n", f.Tag.Get("env"))
		// FieldByName("Host").Tag.Get("env") = "APP_HOST"
	}

	// // Elem(): only meaningful for Slice/Array/Map/Chan/Ptr
	// fmt.Printf("[]int.Elem()  = %v\n", reflect.TypeOf([]int{}).Elem())   // []int.Elem()  = int
	// fmt.Printf("&Cfg.Elem()   = %v\n", reflect.TypeOf(&Config{}).Elem()) // &Cfg.Elem()   = main.Config
	// // Len(): only for Array
	// fmt.Printf("[3]int.Len()  = %d\n", reflect.TypeOf([3]int{}).Len())   // [3]int.Len()  = 3
}

// ===== explainRv: demonstrates common methods of reflect.Value =====
func explainRv(rv reflect.Value) {
	fmt.Println("== reflect.Value (rv) ==")                                  // "== reflect.Value (rv) =="
	fmt.Printf("Kind      = %v\n", rv.Kind())                                // Kind      = struct
	fmt.Printf("Type      = %v\n", rv.Type())                                // Type      = main.Config

	fmt.Printf("CanAddr   = %v  <- only true after Ptr.Elem()\n", rv.CanAddr())
	                                                                         // CanAddr   = true  <- only true after Ptr.Elem()
	fmt.Printf("IsValid   = %v\n", rv.IsValid())                             // IsValid   = true
	fmt.Printf("IsZero    = %v\n", rv.IsZero())                              // IsZero    = false  (Host="127.0.0.1", Port=80)

	fmt.Printf("NumField  = %d\n", rv.NumField())                            // NumField  = 2
	fv := rv.Field(0) // field value (matches rt.Field(0))
	fmt.Printf("Field(0)  = Interface:%v Kind:%v CanSet:%v\n",
		fv.Interface(), fv.Kind(), fv.CanSet())
	// Field(0)  = Interface:127.0.0.1 Kind:string CanSet:true

	// Only writable when addressable
	fv.SetString("0.0.0.0")
	fmt.Printf("SetString afterwards Field(0).Interface() = %v\n", rv.Field(0).Interface())
	// SetString afterwards Field(0).Interface() = 0.0.0.0

	// Value getter (must match Kind, otherwise panic)
	fmt.Printf("Field(1).Int() = %d\n", rv.Field(1).Int())                  // Field(1).Int() = 80

	// Counter-example: passing by value -> not addressable -> CanSet=false, SetXxx would panic
	bad := reflect.ValueOf(Config{})
	fmt.Printf("ValueOf(Config{}).Field(0).CanSet() = %v\n", bad.Field(0).CanSet())
	// ValueOf(Config{}).Field(0).CanSet() = false
}

// ===== envloader: tag-based loader =====
func envloader(vp any) {
	if vp == nil {
		panic("envloader: v is nil")
	}
	rvp := reflect.ValueOf(vp)
	if rvp.Kind() != reflect.Ptr || rvp.IsNil() {
		panic("envloader: must pass a non-nil pointer, e.g. envloader(&cfg)")
	}
	rv := rvp.Elem()
	if rv.Kind() != reflect.Struct {
		panic("envloader: the pointer must point to a struct")
	}
	rt := rv.Type()

	for i := 0; i < rt.NumField(); i++ {
		field := rt.Field(i)
		fv := rv.Field(i)
		if !fv.CanSet() {
			continue
		}

		key := field.Tag.Get("env")
		if key == "" {
			continue
		}
		val, ok := os.LookupEnv(key)
		if !ok {
			continue
		}

		switch fv.Kind() {
		case reflect.String:
			fv.SetString(val)
		case reflect.Int, reflect.Int8, reflect.Int16, reflect.Int32, reflect.Int64:
			n, err := strconv.ParseInt(val, 10, 64)
			if err != nil {
				panic(fmt.Sprintf("envloader: %s(%s=%q) not an integer: %v", field.Name, key, val, err))
			}
			fv.SetInt(n)
		case reflect.Uint, reflect.Uint8, reflect.Uint16, reflect.Uint32, reflect.Uint64:
			n, err := strconv.ParseUint(val, 10, 64)
			if err != nil {
				panic(fmt.Sprintf("envloader: %s(%s=%q) not an unsigned integer: %v", field.Name, key, val, err))
			}
			fv.SetUint(n)
		case reflect.Bool:
			b, err := strconv.ParseBool(val)
			if err != nil {
				panic(fmt.Sprintf("envloader: %s(%s=%q) not a boolean: %v", field.Name, key, val, err))
			}
			fv.SetBool(b)
		default:
			panic(fmt.Sprintf("envloader: field %s of type %s is not supported", field.Name, field.Type))
		}
	}
}

func main() {
	var c Config
	c.Host = "127.0.0.1"
	c.Port = 80

	rv := reflect.ValueOf(&c).Elem()
	rt := rv.Type()

	explainRt(rt)
	fmt.Println()
	explainRv(rv)   // NOTE: this call mutates c.Host -> "0.0.0.0"

	fmt.Println()
	fmt.Println("== envloader ==")   // "== envloader =="
	os.Setenv("APP_HOST", "0.0.0.0")
	os.Setenv("APP_PORT", "8080")
	defer os.Unsetenv("APP_HOST")
	defer os.Unsetenv("APP_PORT")

	envloader(&c)            // must pass a pointer
	fmt.Printf("%+v\n", c)   // {Host:0.0.0.0 Port:8080}
}
