'''
프로그램 실행할 때 매개변수 넣어 주거나 , 기본값으로 처리 예제
'''
import argparse

def run(target: str, id: int):
    print(target)
    print(id)

if __name__ == '__main__':
    print(__name__)

    #매개변수 저장 
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--target', required=False, help='help target', default='default target')
    parser.add_argument('--id',  required=False, type=int, default=0, help='help id')
    args = parser.parse_args()

    #print(args.target)
    #print(args.id)

    run(args.target, args.id)
