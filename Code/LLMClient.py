from openai import OpenAI
import os

class OpenAIClient:
    def __init__(self, api_key, model_name, max_tokens=2000, temperature=0.7):
        self.client = OpenAI(api_key=api_key)
        self.model_name = model_name
        self.max_tokens = max_tokens
        self.temperature = temperature

    def get_response(self, prompt):
        response = self.client.chat.completions.create(
            model=self.model_name,
            messages=[
                {"role": "system", "content": "You are a helpful C++ coding assistant."},
                {"role": "user", "content": prompt}
            ],
            max_tokens=self.max_tokens,
            temperature=self.temperature
        )
        return response.choices[0].message.content
