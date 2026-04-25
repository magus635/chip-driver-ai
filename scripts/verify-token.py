#!/usr/bin/env python3
import os
import sys
import json
import hashlib
import subprocess
from datetime import datetime

def get_tree_hash(directory):
    try:
        files = subprocess.check_output(['git', 'ls-files', directory], text=True).splitlines()
        hasher = hashlib.sha256()
        for f in sorted(files):
            if os.path.isfile(f):
                with open(f, 'rb') as fd:
                    hasher.update(fd.read())
        return hasher.hexdigest()
    except:
        hasher = hashlib.sha256()
        for root, dirs, files_list in os.walk(directory):
            for f in sorted(files_list):
                fpath = os.path.join(root, f)
                with open(fpath, 'rb') as fd:
                    hasher.update(fd.read())
        return hasher.hexdigest()

def verify_token(consume=False):
    token_path = ".claude/reviewer-pass.token"
    
    if not os.path.exists(token_path):
        print("[ERROR] reviewer-pass.token not found. Run reviewer-agent first.")
        sys.exit(3)
    
    with open(token_path, 'r') as f:
        token = json.load(f)
    
    if token.get('consumed'):
        print("[ERROR] Token already consumed. Re-run reviewer-agent.")
        sys.exit(3)
    
    # 校验 rules_version
    current_rules_version = subprocess.check_output(['bash', 'scripts/rules-version.sh'], text=True).strip()
    if token.get('rules_version') != current_rules_version:
        print(f"[ERROR] Rules version drift: token({token.get('rules_version')}) vs current({current_rules_version})")
        sys.exit(3)
        
    # 校验 Hash
    current_src_hash = get_tree_hash('src')
    current_ir_hash = get_tree_hash('ir')
    
    if token.get('src_tree_hash') != current_src_hash or token.get('ir_tree_hash') != current_ir_hash:
        print("[ERROR] Code or IR changed since review. Re-run reviewer-agent.")
        sys.exit(3)
        
    if consume:
        token['consumed'] = True
        token['consumed_at'] = datetime.utcnow().isoformat() + "Z"
        with open(token_path, 'w') as f:
            json.dump(token, f, indent=2)
        print("Token verified and consumed.")
    else:
        print("Token verified (dry-run).")
    
    sys.exit(0)

if __name__ == "__main__":
    consume = "--consume" in sys.argv
    verify_token(consume)
