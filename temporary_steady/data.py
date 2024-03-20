import pandas as pd

# 读取文件
def read_file(file_path):
    with open(file_path, 'r') as file:
        return file.read().splitlines()

# 解析数据
def parse_data(lines):
    data = {}
    current_sketch = None
    current_memory = None
    for line in lines:
        if line.startswith('memory:'):
            current_memory = line.split(': ')[1]
        elif line.startswith('sketch:'):
            current_sketch = line.split(': ')[1]
            if current_sketch not in data:
                data[current_sketch] = []
        elif current_sketch and current_memory:
            if ":" in line:  # 处理键值对
                key, value = line.split(': ')
                if key in ['throughput', 'MSE', 'group', 'RR', 'PR', 'before duplicate remove', 'after duplicate remove']:
                    # 如果当前行是新的度量开始，首先确保有一个新的字典
                    if not (data[current_sketch] and 'memory' in data[current_sketch][-1] and data[current_sketch][-1]['memory'] == current_memory):
                        data[current_sketch].append({'memory': current_memory})
                    data[current_sketch][-1][key] = value
            else:
                # 处理不包含":"的行，比如单独的数字
                if data[current_sketch]:
                    last_key = list(data[current_sketch][-1].keys())[-1]  # 获取最后一个键，确保不会出现越界错误
                    if last_key:  # 如果存在最后一个键，则添加或更新值
                        if last_key.endswith('remove'):
                            data[current_sketch][-1][last_key + ' value'] = line
    return data


# 保存为CSV
def save_to_csv(data):
    for sketch, entries in data.items():
        df = pd.DataFrame(entries)
        # df = df.set_index('memory').sort_index()  # 设置memory为索引并排序
        df['memory'] = df['memory'].astype(int)
        df = df.sort_values(by='memory')  # 按memory值排序
        csv_file = f'{sketch.replace(" ", "_")}.csv'
        df.to_csv('sync/'+csv_file, index=False)
        print(f'Saved {csv_file}')

# 主函数
def main():
    file_path = 'sync.txt'  # 需要根据实际路径调整
    lines = read_file(file_path)
    data = parse_data(lines)
    save_to_csv(data)

if __name__ == "__main__":
    main()
