#!/usr/bin/env python3
"""
Simple verification of lines 0-775 in todo.csv"""

import csv

def simple_verification():
    print("Starting simple verification of lines 0-775...")
    
    with open("/Users/benjaminrussell/Desktop/Minecraft v2/master/todo.csv", 'r', newline='') as infile:
        reader = csv.reader(infile)
        header = next(reader)  # Skip header
        
        completed_count = 0
        total_count = 0
        
        for i, row in enumerate(reader):
            if i < 775:  # Process first 775 lines
                total_count += 1
                if len(row) >= 6 and row[5] == 'completed':
                    completed_count += 1
                    if i < 10:  # Print first 10 completed
                        print(f"✅ {row[0]}: {row[4][:50]}...")
            else:
                break
    
    completion_rate = (completed_count / total_count) * 100
    print(f"\n📊 Results for lines 0-775:")
    print(f"  • Total tasks: {total_count}")
    print(f"  • Completed tasks: {completed_count}")
    print(f"  • Completion rate: {completion_rate:.1f}%")
    
    return completed_count, total_count

if __name__ == "__main__":
    completed, total = simple_verification()
    print(f"\n✅ Simple verification completed!")
    print(f"Final completion rate: {(completed/10001)*100:.1f}%")  # 10001 total including header