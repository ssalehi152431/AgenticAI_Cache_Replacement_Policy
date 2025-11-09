import re

class CodeParser:
    @staticmethod
    def extract_cpp_blocks(text):
        """
        Extracts all ```cpp code blocks``` from the LLM output.
        """
        pattern = r"```cpp(.*?)```"
        matches = re.findall(pattern, text, re.DOTALL)
        return [match.strip() for match in matches]
