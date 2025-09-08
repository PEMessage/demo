import lombok.Data;
import lombok.AllArgsConstructor;
import lombok.NoArgsConstructor;
import lombok.Builder;

@Data // Generates getters, setters, toString, equals, hashCode
@Builder // Generates a builder pattern
@NoArgsConstructor // Generates a no-args constructor
@AllArgsConstructor // Generates an all-args constructor
public class ExampleAll {
    private Long id;
    private String firstName;
    private String lastName;
    private String email;
}
