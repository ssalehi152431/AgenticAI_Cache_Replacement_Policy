import os
from dotenv import load_dotenv
from LLMClient import OpenAIClient
from PromptBuilder import PromptBuilder
from CodeParser import CodeParser
from FileManager import FileManager
from utils import setup_logger, validate_api_key, get_file_content, record_score, invoke_simulator, save_to_database

def main():
    # Setup logging
    logger = setup_logger()

    # Load environment variables
    load_dotenv()
    api_key = os.getenv("OPENAI_API_KEY")
    model_name = os.getenv("MODEL_NAME")
    input_dir = os.getenv("INPUT_SCRIPTS_DIR")
    output_dir = os.getenv("OUTPUT_SCRIPTS_DIR")
    score_hist = os.getenv("SCORE_HISTORY")
    file_name_prefix = os.getenv("FILE_NAME_PREFIX")
    validate_key = os.getenv("ENABLE_API_KEY_VALIDATION", "False") == "True"
    max_tokens = int(os.getenv("MAX_TOKENS", 10000))
    temperature = float(os.getenv("TEMPERATURE", 0.7))
    threshold = float(os.getenv("TRACE_SCORE"))
    iteration = int(os.getenv("ITERATION", 5))
    simulation_timeout = int(os.getenv("CHAMPSIM_TIMEOUT", 240))

    if validate_key:
        if not validate_api_key(api_key):
            logger.error("API key validation failed.")
            return

    try:
        prompter = PromptBuilder(input_dir)
        client = OpenAIClient(api_key, model_name, max_tokens, temperature)
        file_manager = FileManager(output_dir, file_name_prefix)

        prompt = prompter.build_prompt()
        logger.info(f"Prompt:\n{prompt}")
        full_response = client.get_response(prompt)
        logger.info(f"Full LLM Response:\n{full_response}")
        code_blocks = CodeParser.extract_cpp_blocks(full_response)
        file_manager.save_code_blocks(code_blocks)
        logger.info(f"Successfully generated {len(code_blocks)} C++ files.")

        generated_files = os.listdir(output_dir)
        generated_files = list(filter(lambda f: f.endswith((".cpp", ".cc", ".h")), generated_files))
        filename = generated_files[0]

        for i in range(iteration):
            success, trace, type, value = invoke_simulator(simulation_timeout, filename)
            logger.info(f"Simulated. trace: {trace}, type: {type}, value: {value}")
            record_score(trace, type, value, score_hist)
            code = get_file_content(f"{output_dir}/{filename}")
            if i == iteration - 1:
                logger.info(f"Final Simulation Result: trace: {trace}, type: {type}, value: {value}")
                break
            if success:
                if code:
                    if type == "Error":
                        prompt = prompter.fix_code_prompt(code, value)
                        logger.info(f"Error Fix Prompt:\n{prompt}")
                        full_response = client.get_response(prompt)
                        logger.info(f"Full LLM Response:\n{full_response}")
                        code_blocks = CodeParser.extract_cpp_blocks(full_response)
                        file_manager.save_single_code_block(code_blocks, filename)
                        logger.info(f"Successfully generated C++ files.")
                    if type == "HitRate":
                        logger.info(f"Successfully simulated. Cache Hit Rate: {value}")
                        if float(value) >= threshold:
                            save_to_database(filename, value, input_dir, output_dir)
                        prompt = prompter.build_prompt()
                        logger.info(f"Improvement Prompt:\n{prompt}")
                        full_response = client.get_response(prompt)
                        logger.info(f"Full LLM Response:\n{full_response}")
                        code_blocks = CodeParser.extract_cpp_blocks(full_response)
                        file_manager.save_single_code_block(code_blocks, filename)
                        logger.info(f"Successfully generated C++ files.")
            else:
                if type == "Timeout":
                    if code:
                        logger.error("Simulation Timeout")
                        prompt = prompter.fix_timeout_prompt(code)
                        logger.info(f"Timeout Fix Prompt:\n{prompt}")
                        full_response = client.get_response(prompt)
                        logger.info(f"Full LLM Response:\n{full_response}")
                        code_blocks = CodeParser.extract_cpp_blocks(full_response)
                        file_manager.save_single_code_block(code_blocks, filename)
                        logger.info(f"Successfully generated C++ files.")
                else:
                    logger.error("Simulation Failed")


    except Exception as e:
        logger.exception(f"An error occurred: {e}")


if __name__ == "__main__":
    main()
