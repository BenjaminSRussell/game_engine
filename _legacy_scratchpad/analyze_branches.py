import subprocess
import sys

def run_command(command):
    try:
        result = subprocess.run(command, shell=True, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        return result.stdout.strip()
    except subprocess.CalledProcessError as e:
        return ""

def get_jules_branches():
    # Get all remote branches with their last committer
    # Format: refname authorname
    cmd = "git for-each-ref --format='%(refname:short)|%(authorname)' refs/remotes/origin"
    output = run_command(cmd)
    
    jules_branches = []
    for line in output.split('\n'):
        if "|" not in line: continue
        ref, author = line.split('|', 1)
        if "jules" in author.lower():
            jules_branches.append(ref)
    return jules_branches

def get_diff_stats(branch):
    # Get stats of changes that the branch would introduce if merged
    # We use ... to find the diff from the merge base
    cmd = f"git diff --shortstat HEAD...{branch}"
    output = run_command(cmd)
    
    # Output format example: " 1 file changed, 10 insertions(+), 5 deletions(-)"
    insertions = 0
    deletions = 0
    
    if not output:
        return 0, 0
        
    parts = output.split(',')
    for part in parts:
        if "insertion" in part:
            insertions = int(part.strip().split()[0])
        elif "deletion" in part:
            deletions = int(part.strip().split()[0])
            
    return insertions, deletions

def main():
    branches = get_jules_branches()
    print(f"Found {len(branches)} branches by Jules.")
    
    valid_branches = []
    
    print(f"{'Branch':<60} {'Ins':<10} {'Del':<10} {'Net':<10}")
    print("-" * 90)
    
    for branch in branches:
        ins, dels = get_diff_stats(branch)
        net = ins - dels
        print(f"{branch:<60} {ins:<10} {dels:<10} {net:<10}")
        if net > 0:
            valid_branches.append(branch)
            
    print("-" * 90)
    print("Candidates to merge:")
    for b in valid_branches:
        print(b)

if __name__ == "__main__":
    main()
