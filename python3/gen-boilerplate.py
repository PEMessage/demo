import argparse

def generate_class(class_name, fields):
    print(f"public class {class_name} {{")
    print("    private Map<String, Object> data;\n")
    print(f"    public {class_name}() {{")
    print("        data = new HashMap<>();")
    print("    }\n")

    for field, T in fields:
        param = field[0].lower() + field[1:]
        print(f"    public void set{field}({T} {param}) {{")
        print(f'        data.put("{field}", {param});')
        print("    }\n")

        print(f"    public {T} get{field}() {{")
        print(f'        return {T} data.get("{field}");')
        print("    }\n")

    print('    public Object getValueFromData(String key) {')
    print('        if(data.containsKey(key)){')
    print('            return data.get(key);')
    print('        }')
    print('        return null;')
    print('    }')
    print("}")

def parse_fields(field_args):
    fields = []
    for field_arg in field_args:
        try:
            name, type_ = field_arg.split(':')
            fields.append((name, type_))
        except ValueError:
            raise argparse.ArgumentTypeError(f"Field '{field_arg}' should be in format 'name:type'")
    return fields

def main():
    parser = argparse.ArgumentParser(description='Generate a Java class with specified fields.')
    parser.add_argument('--class', dest='class_name', default='A',
                       help='Name of the class to generate')
    parser.add_argument('--fields', nargs='+', required=True,
                       type=lambda x: parse_fields([x])[0],
                       help='Fields in format "FieldName:Type" (e.g., "Age:int")')
    
    args = parser.parse_args()
    
    generate_class(args.class_name, args.fields)

if __name__ == "__main__":
    main()
