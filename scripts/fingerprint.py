#!/usr/bin/env python3
import sys
import hashlib
import re

def normalize(text):
    if not text:
        return ""
    # 转换为小写，去掉首尾空格
    text = text.strip().lower()
    # 进一步的规范化逻辑（可选）：移除文件路径等
    return text

def calculate_fingerprint(error_code, symbol, phase, root_cause):
    raw_str = (normalize(error_code) + 
               normalize(symbol) + 
               normalize(phase) + 
               normalize(root_cause))
    
    sha1 = hashlib.sha1(raw_str.encode('utf-8')).hexdigest()
    return sha1[:12]

if __name__ == "__main__":
    if len(sys.argv) < 5:
        print("Usage: fingerprint.py <error_code> <symbol> <phase> <root_cause>")
        sys.exit(1)
    
    error_code = sys.argv[1]
    symbol = sys.argv[2]
    phase = sys.argv[3]
    root_cause = sys.argv[4]
    
    print(calculate_fingerprint(error_code, symbol, phase, root_cause))
