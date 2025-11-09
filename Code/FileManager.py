import os

class FileManager:
    def __init__(self, output_dir, file_prefix):
        self.output_dir = output_dir
        self.file_prefix = file_prefix
        os.makedirs(self.output_dir, exist_ok=True)

    def save_code_blocks(self, code_blocks):
        for idx, code in enumerate(code_blocks, 1):
            filename = f"{self.file_prefix}_{idx}.cc"
            filepath = os.path.join(self.output_dir, filename)
            with open(filepath, "w") as f:
                f.write(code)

    def save_single_code_block(self, code_block, filename):
        for idx, code in enumerate(code_block, 1):
            filepath = os.path.join(self.output_dir, filename)
            with open(filepath, "w") as f:
                f.write(code)
