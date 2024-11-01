import re

enable_load_parse = 1
enable_run_parse = 1
if enable_load_parse == 1:
    with open('YCSB_load_trace.txt', 'w') as load:
        load.write("30000\n")
        for i in range(1, 400001):
        # 写入当前行的数字，1 和 i，然后换行
            load.write(f"1 {i}\n")
        # with open('load.txt', 'r') as f:
        #     lines = f.readlines()
        #     for line in lines:
        #         simple_matches = re.findall(r'field(\d+)=([^"\[\]\s]+.*?)(?=\s*field\d+=|$)', line)  
                
        #         for field_number, value in simple_matches:
        #             load.write('1'+ ' ' + field_number + ' ' + '\n')
        #             # load.write('1'+ ' ' + field_number + ' ' + field_number + '\n')
        #             # load.write('1'+ ' ' + field_number + ' ' + field_number + ' ' + value + '\n')
        # f.close()
    load.close()


run = []
if enable_run_parse == 1:
    with open('YCSB_run_trace.txt', 'w') as load:
        load.write("40000\n")
        print(2)
        with open('run.txt', 'r') as f:
            lines = f.readlines()
            for line in lines:  
                line = line.strip()
                if line.startswith('UPDATE') or line.startswith('READ'): 
                    run.append(line)

            # for line in lines:
            #     simple_matches = re.findall(r'field(\d+)=([^"\[\]\s]+.*?)(?=\s*field\d+=|$)', line)  
                
            #     for field_number, value in simple_matches:  
            #         load.write('1'+ ' ' + field_number + ' ' + value + '\n')
        f.close()
        ## 
        for item in run:
            if item.startswith('UPDATE'):
                simple_matches = re.findall(r'field(\d+)=([^"\[\]\s]+.*?)(?=\s*field\d+=|$)', item)  
                for field_number, value in simple_matches:  
                    # load.write('1'+ ' ' + field_number + ' ' + field_number + ' ' + value + '\n')
                    # load.write('1'+ ' ' + field_number + ' ' + field_number + '\n')
                    load.write('1'+ ' ' + field_number + '\n')
            elif item.startswith('READ'):  
                # 查找 'field' 关键字并截取后面的数字  
                start_index = item.find('field') + len('field')  
                end_index = item.find(']', start_index)  
                # 提取并转换数字  
                field_number = item[start_index:end_index]  
                # 如果需要，将字符串转换为整数
                field_number_int = int(field_number)
                # load.write('0'+ ' ' + field_number + ' ' + field_number + '\n')
                load.write('0'+ ' ' + field_number + ' ' + '\n')
                # print(f"field{field_number} = {field_number}")  # 如果你只是想要输出格式，而不是实际的变量名
        # print(run)
    load.close()

