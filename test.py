import os
import subprocess

# datasets2 = ['download_std30.txt','download_std40.txt','download_std50.txt','download_std60.txt','download_std70.txt']
# datasets3 = ['meanUpload1000.txt','meanUpload900.txt','meanUpload800.txt','meanUpload700.txt','meanUpload600.txt']
# datasets4 = ['meanDownload1000.txt','meanDownload900.txt','meanDownload800.txt','meanDownload700.txt','meanDownload600.txt']

datasets = [
            ['download_std40.txt','download_std50.txt','download_std60.txt','download_std70.txt'],
            ['meanUpload900.txt','meanUpload800.txt','meanUpload700.txt','meanUpload600.txt'],
            ['upload_std40.txt','upload_std50.txt','upload_std60.txt','upload_std70.txt'],
            ['meanDownload900.txt','meanDownload800.txt','meanDownload700.txt','meanDownload600.txt'],
            ['minDownload0.1.txt','minDownload0.2.txt','minDownload0.3.txt','minDownload0.4.txt'],
            ['minUpload0.1.txt','minUpload0.2.txt','minUpload0.3.txt','minUpload0.4.txt']
            ]

for datas in datasets:
    for dataset in datas:
        outputfile = 'dynamic2/' + dataset
        cmd = 'cp ' + 'conf/bandwidth/dynamic/' + dataset + ' conf/bandwidth/bandwidth.txt'
        print(cmd)
        os.system(cmd)
        cmd = 'build/SingleCoordinator dynamic Clay-1410256-0-0 true '
        # print(cmd)
        # os.system(cmd)
        # output = subprocess.check_output(cmd,shell=True).decode()
        with open(outputfile,'w') as file:
            
            p = subprocess.Popen(cmd, shell=True,stdout = subprocess.PIPE, stderr = subprocess.PIPE)
            for line in iter(p.stdout.readline, b''):
                file.write(line.decode('utf-8'))
                # print(line.decode('utf-8'))
            p.stdout.close()

for datas in datasets:
    for dataset in datas:
        outputfile = 'affinaty/' + dataset
        cmd = 'cp ' + 'conf/bandwidth/dynamic/' + dataset + ' conf/bandwidth/bandwidth.txt'
        # print(cmd)
        os.system(cmd)
        cmd = 'build/SingleCoordinator affinaty Clay-1410256-0-0 true '
        # print(cmd)
        # os.system(cmd)
        # output = subprocess.check_output(cmd,shell=True).decode()
        with open(outputfile,'w') as file:
            
            p = subprocess.Popen(cmd, shell=True,stdout = subprocess.PIPE, stderr = subprocess.PIPE)
            for line in iter(p.stdout.readline, b''):
                file.write(line.decode('utf-8'))
                # print(line.decode('utf-8'))
            p.stdout.close()

for datas in datasets:
    for dataset in datas:
        outputfile = 'cent/' + dataset
        cmd = 'cp ' + 'conf/bandwidth/dynamic/' + dataset + ' conf/bandwidth/bandwidth.txt'
        # print(cmd)
        os.system(cmd)
        cmd = 'build/SingleCoordinator centralize Clay-1410256-0-0 true '
        # print(cmd)
        # os.system(cmd)
        # output = subprocess.check_output(cmd,shell=True).decode()
        with open(outputfile,'w') as file:
            
            p = subprocess.Popen(cmd, shell=True,stdout = subprocess.PIPE, stderr = subprocess.PIPE)
            for line in iter(p.stdout.readline, b''):
                file.write(line.decode('utf-8'))
                # print(line.decode('utf-8'))
            p.stdout.close()

# for dataset in datasets3:
#     outputfile = 'dynamic/' + dataset
#     cmd = 'cp ' + 'conf/bandwidth/dynamic/' + dataset + ' conf/bandwidth/bandwidth.txt'
#     print(cmd)
#     os.system(cmd)
#     cmd = 'build/SingleCoordinator dynamic Clay-1410256-0-0 true '
#     # print(cmd)
#     # os.system(cmd)
#     # output = subprocess.check_output(cmd,shell=True).decode()
#     with open(outputfile,'w') as file:
        
#         p = subprocess.Popen(cmd, shell=True,stdout = subprocess.PIPE, stderr = subprocess.PIPE)
#         for line in iter(p.stdout.readline, b''):
#             file.write(line.decode('utf-8'))
#             # print(line.decode('utf-8'))
#         p.stdout.close()

# for dataset in datasets2:
#     outputfile = 'dynamic/' + dataset
#     cmd = 'cp ' + 'conf/bandwidth/dynamic/' + dataset + ' conf/bandwidth/bandwidth.txt'
#     print(cmd)
#     os.system(cmd)
#     cmd = 'build/SingleCoordinator dynamic Clay-1410256-0-0 true '
#     # print(cmd)
#     # os.system(cmd)
#     # output = subprocess.check_output(cmd,shell=True).decode()
#     with open(outputfile,'w') as file:
        
#         p = subprocess.Popen(cmd, shell=True,stdout = subprocess.PIPE, stderr = subprocess.PIPE)
#         for line in iter(p.stdout.readline, b''):
#             file.write(line.decode('utf-8'))
#             # print(line.decode('utf-8'))
#         p.stdout.close()

# for dataset in datasets4:
#     outputfile = 'dynamic/' + dataset
#     cmd = 'cp ' + 'conf/bandwidth/dynamic/' + dataset + ' conf/bandwidth/bandwidth.txt'
#     print(cmd)
#     os.system(cmd)
#     cmd = 'build/SingleCoordinator dynamic Clay-1410256-0-0 true '
#     # print(cmd)
#     # os.system(cmd)
#     # output = subprocess.check_output(cmd,shell=True).decode()
#     with open(outputfile,'w') as file:
        
#         p = subprocess.Popen(cmd, shell=True,stdout = subprocess.PIPE, stderr = subprocess.PIPE)
#         for line in iter(p.stdout.readline, b''):
#             file.write(line.decode('utf-8'))
#             # print(line.decode('utf-8'))
#         p.stdout.close()