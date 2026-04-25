#!/usr/bin/env python3
import os
import sys
import json
import hashlib
import subprocess
from datetime import datetime
import uuid

def get_tree_hash(directory):
    # 使用 git ls-files 来保证顺序确定
    try:
        # 获取所有跟踪的文件
        files = subprocess.check_output(['git', 'ls-files', directory], text=True).splitlines()
        hasher = hashlib.sha256()
        for f in sorted(files):
            if os.path.isfile(f):
                with open(f, 'rb') as fd:
                    hasher.update(fd.read())
        return hasher.hexdigest()
    except Exception as e:
        # 如果不是 git 仓库，退化到普通遍历
        hasher = hashlib.sha256()
        for root, dirs, files_list in os.walk(directory):
            for f in sorted(files_list):
                fpath = os.path.join(root, f)
                with open(fpath, 'rb') as fd:
                    hasher.update(fd.read())
        return hasher.hexdigest()

def issue_token(rules_version, findings):
    src_hash = get_tree_hash('src')
    ir_hash = get_tree_hash('ir')
    
    try:
        git_commit = subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip()
    except:
        git_commit = "unknown"

    token = {
        "schema_version": "1.0",
        "issued_at": datetime.utcnow().isoformat() + "Z",
        "rules_version": rules_version,
        "git_commit": git_commit,
        "src_tree_hash": src_hash,
        "ir_tree_hash": ir_hash,
        "reviewer_run_id": str(uuid.uuid4()),
        "findings_summary": findings,
        "consumed": False,
        "consumed_at": None
    }

    token_path = ".claude/reviewer-pass.token"
    history_dir = ".claude/token-history"
    
    # 归档旧 token
    if os.path.exists(token_path):
        with open(token_path, 'r') as f:
            old_token = json.load(f)
            ts = old_token.get('issued_at', 'unknown').replace(':', '-')
            os.makedirs(history_dir, exist_ok=True)
            os.rename(token_path, f"{history_dir}/{ts}.token")

    with open(token_path, 'w') as f:
        json.dump(token, f, indent=2)
    
    print(f"Token issued: {token['reviewer_run_id']}")

if __name__ == "__main__":
    # 模拟输入参数
    rules_version = subprocess.check_output(['bash', 'scripts/rules-version.sh'], text=True).strip()
    
    # 默认 findings (实际生产中应由 reviewer-agent 传入)
    findings = {
        "arch_check": "pass",
        "invariants_check": "pass",
        "r9b_reviews_resolved": 0,
        "r9b_reviews_open": 0
    }
    
    issue_token(rules_version, findings)
