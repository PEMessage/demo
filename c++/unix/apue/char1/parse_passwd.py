def parse_and_print_passwd():
    try:
        with open('/etc/passwd', 'r') as file:
            print("\n{:<20} {:<10} {:<10} {:<10} {:<40} {:<20} {:<10}".format(
                "Username", "passwd",  "UID", "GID", "Description", "Home Directory", "Shell"
            ))
            print("-" * 110)
            
            for line in file:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                
                parts = line.split(':')
                if len(parts) >= 7:
                    username = parts[0]
                    passwd = parts[1]
                    uid = parts[2]
                    gid = parts[3]
                    description = parts[4] if parts[4] else "None"
                    home_dir = parts[5]
                    shell = parts[6]
                    
                    print("{:<20} {:<10} {:<10} {:<10} {:<40} {:<20} {:<10}".format(
                        username, passwd, uid, gid, description, home_dir, shell
                    ))
    
    except FileNotFoundError:
        print("Error: /etc/passwd file not found. This program only works on Unix-like systems.")
    except PermissionError:
        print("Error: Permission denied. You need appropriate permissions to read /etc/passwd.")
    except Exception as e:
        print(f"An unexpected error occurred: {str(e)}")

if __name__ == "__main__":
    parse_and_print_passwd()
