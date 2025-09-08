import lombok.Data;

@Data // Generates getters, setters, toString, equals, hashCode
public class Example {
    private Long id;
    private String firstName;
    private String lastName;
    private String email;
}
